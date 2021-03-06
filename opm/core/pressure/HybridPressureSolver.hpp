/*
  Copyright 2010 SINTEF ICT, Applied Mathematics.

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef OPM_HYBRIDPRESSURESOLVER_HEADER_INCLUDED
#define OPM_HYBRIDPRESSURESOLVER_HEADER_INCLUDED

#include <opm/core/pressure/fsh.h>
#include <opm/core/linalg/sparse_sys.h>
#include <opm/core/pressure/mimetic/mimetic.h>
#include <opm/core/GridAdapter.hpp>
#include <stdexcept>


/// @brief
/// Encapsulates the ifsh (= incompressible flow solver hybrid) solver modules.
class HybridPressureSolver
{
public:
    /// @brief
    /// Default constructor, does nothing.
    HybridPressureSolver()
        :  state_(Uninitialized), data_(0)
    {
    }

    /// @brief
    /// Destructor.
    ~HybridPressureSolver()
    {
        fsh_destroy(data_);
    }

    /// @brief
    /// Initialize the solver's structures for a given grid (at some point also well pattern).
    /// @tparam Grid This must conform to the SimpleGrid concept.
    /// @param grid The grid object.
    /// @param perm Permeability. It should contain dim*dim entries (a full tensor) for each cell.
    /// @param gravity Array containing gravity acceleration vector. It should contain dim entries.
    template <class Grid>
    void init(const Grid& grid, const double* perm, const double* gravity)
    {
        // Build C grid structure.
        grid_.init(grid);

        // Checking if grids are properly initialized. Move this code to a test somewhere.
//         GridAdapter grid2;
//         grid2.init(grid_);
//         if (grid2 == grid_) {
//             std::cout << "Grids are equal." << std::endl;
//         } else {
//             std::cout << "Grids are NOT equal." << std::endl;
//         }

        // Build (empty for now) C well structure.
        well_t* w = 0;

        // Initialize ifsh data.
        data_ = ifsh_construct(grid_.c_grid(), w);
        if (!data_) {
            throw std::runtime_error("Failed to initialize ifsh solver.");
        }

        // Compute inner products, gravity contributions.
        int num_cells = grid.numCells();
        int ngconn  = grid_.c_grid()->cell_facepos[num_cells];
        gpress_.clear();
        gpress_.resize(ngconn, 0.0);
        int ngconn2 = data_->sum_ngconn2;
        Binv_.resize(ngconn2);
        ncf_.resize(num_cells);
        typename Grid::Vector grav;
        std::copy(gravity, gravity + Grid::dimension, &grav[0]);
        int count = 0;
        for (int cell = 0; cell < num_cells; ++cell) {
            int num_local_faces = grid.numCellFaces(cell);
            ncf_[cell] = num_local_faces;
            typename Grid::Vector cc = grid.cellCentroid(cell);
            for (int local_ix = 0; local_ix < num_local_faces; ++local_ix) {
                int face = grid.cellFace(cell, local_ix);
                typename Grid::Vector fc = grid.faceCentroid(face);
                gpress_[count++] = grav*(fc - cc);
            }
        }
        assert(count == ngconn);

        UnstructuredGrid* g = grid_.c_grid();
        mim_ip_simple_all(g->number_of_cells, g->dimensions,
                          data_->max_ngconn,
                          g->cell_facepos, g->cell_faces,
                          g->face_cells, g->face_centroids,
                          g->face_normals, g->face_areas,
                          g->cell_centroids, g->cell_volumes,
                          const_cast<double*>(perm), &Binv_[0]);
        state_ = Initialized;
    }


    enum FlowBCTypes { FBC_UNSET, FBC_PRESSURE, FBC_FLUX };

    /// @brief
    /// Assemble the sparse system.
    /// You must call init() prior to calling assemble().
    /// @param sources Source terms, one per cell. Positive numbers
    /// are sources, negative are sinks.
    /// @param total_mobilities Scalar total mobilities, one per cell.
    /// @param omegas Gravity term, one per cell. In a multi-phase
    /// flow setting this is equal to
    /// \f[ \omega = \sum_{p} \frac{\lambda_p}{\lambda_t} \rho_p \f]
    /// where \f$\lambda_p\f$ is a phase mobility, \f$\rho_p\f$ is a
    /// phase density and \f$\lambda_t\f$ is the total mobility.
    void assemble(const std::vector<double>& sources,
                  const std::vector<double>& total_mobilities,
                  const std::vector<double>& omegas,
                  const std::vector<FlowBCTypes>& bctypes,
                  const std::vector<double>& bcvalues)
    {
        if (state_ == Uninitialized) {
            throw std::runtime_error("Error in HybridPressureSolver::assemble(): You must call init() prior to calling assemble().");
        }

        // Boundary conditions.

        assert (bctypes.size() ==
                static_cast<std::vector<FlowBCTypes>::size_type>(grid_.numFaces()));

        FlowBoundaryConditions *bc = gather_boundary_conditions(bctypes, bcvalues);

        // Source terms from user.
        double* src = const_cast<double*>(&sources[0]); // Ugly? Yes. Safe? I think so.

        // All well related things are zero.
        well_control_t* wctrl = 0;
        double* WI = 0;
        double* wdp = 0;

        // Scale inner products and gravity terms by saturation-dependent factors.
        UnstructuredGrid* g = grid_.c_grid();
        Binv_mobilityweighted_.resize(Binv_.size());
        mim_ip_mobility_update(g->number_of_cells, g->cell_facepos, &total_mobilities[0],
                               &Binv_[0], &Binv_mobilityweighted_[0]);
        gpress_omegaweighted_.resize(gpress_.size());
        mim_ip_density_update(g->number_of_cells, g->cell_facepos, &omegas[0],
                              &gpress_[0], &gpress_omegaweighted_[0]);


        // Zero the linalg structures.
        csrmatrix_zero(data_->A);
        for (std::size_t i = 0; i < data_->A->m; i++) {
            data_->b[i] = 0.0;
        }

        // Assemble the embedded linear system.
        ifsh_assemble(bc, src, &Binv_mobilityweighted_[0], &gpress_omegaweighted_[0],
                      wctrl, WI, wdp, data_);
        state_ = Assembled;

        flow_conditions_destroy(bc);
    }

    /// Encapsulate a sparse linear system in CSR format.
    struct LinearSystem
    {
        int n;
        int nnz;
        int* ia;
        int* ja;
        double* sa;
        double* b;
        double* x;
    };

    /// @brief
    /// Access the linear system assembled.
    /// You must call assemble() prior to calling linearSystem().
    /// @param[out] s The linear system encapsulation to modify.
    /// After this call, s will point to linear system structures
    /// that are owned and allocated internally.
    void linearSystem(LinearSystem& s)

    {
        if (state_ != Assembled) {
            throw std::runtime_error("Error in HybridPressureSolver::linearSystem(): "
                                     "You must call assemble() prior to calling linearSystem().");
        }
        s.n = data_->A->m;
        s.nnz = data_->A->nnz;
        s.ia = data_->A->ia;
        s.ja = data_->A->ja;
        s.sa = data_->A->sa;
        s.b = data_->b;
        s.x = data_->x;
    }

    /// @brief
    /// Compute cell pressures and face fluxes.
    /// You must call assemble() (and solve the linear system accessed
    /// by calling linearSystem()) prior to calling
    /// computePressuresAndFluxes().
    /// @param[out] cell_pressures Cell pressure values.
    /// @param[out] face_areas Face flux values.
    void computePressuresAndFluxes(std::vector<double>& cell_pressures,
                                   std::vector<double>& face_fluxes)
    {
        if (state_ != Assembled) {
            throw std::runtime_error("Error in HybridPressureSolver::computePressuresAndFluxes(): "
                                     "You must call assemble() (and solve the linear system) "
                                     "prior to calling computePressuresAndFluxes().");
        }
        int num_cells = grid_.c_grid()->number_of_cells;
        int num_faces = grid_.c_grid()->number_of_faces;
        cell_pressures.clear();
        cell_pressures.resize(num_cells, 0.0);
        face_fluxes.clear();
        face_fluxes.resize(num_faces, 0.0);
        fsh_press_flux(grid_.c_grid(), &Binv_mobilityweighted_[0], &gpress_omegaweighted_[0],
                       data_, &cell_pressures[0], &face_fluxes[0], 0, 0);
    }

    /// @brief
    /// Compute cell fluxes from face fluxes.
    /// You must call assemble() (and solve the linear system accessed
    /// by calling linearSystem()) prior to calling
    /// faceFluxToCellFlux().
    /// @param face_fluxes 
    /// @param face_areas Face flux values (usually output from computePressuresAndFluxes()).
    /// @param[out] cell_fluxes Cell-wise flux values.
    /// They are given in cell order, and for each cell there is
    /// one value for each adjacent face (in the same order as the
    /// cell-face topology of the grid). Positive values represent
    /// fluxes out of the cell.
    void faceFluxToCellFlux(const std::vector<double>& face_fluxes,
                            std::vector<double>& cell_fluxes)
    {
        if (state_ != Assembled) {
            throw std::runtime_error("Error in HybridPressureSolver::faceFluxToCellFlux(): "
                                     "You must call assemble() (and solve the linear system) "
                                     "prior to calling faceFluxToCellFlux().");
        }
        const UnstructuredGrid& g = *(grid_.c_grid());
        int num_cells = g.number_of_cells;
        cell_fluxes.resize(g.cell_facepos[num_cells]);
        for (int cell = 0; cell < num_cells; ++cell) {
            for (int hface = g.cell_facepos[cell]; hface < g.cell_facepos[cell + 1]; ++hface) {
                int face = g.cell_faces[hface];
                bool pos = (g.face_cells[2*face] == cell);
                cell_fluxes[hface] = pos ? face_fluxes[face] : -face_fluxes[face];
            }
        }
    }

    /// @brief
    /// Access the number of connections (faces) per cell. Deprecated, will be removed.
    const std::vector<int>& numCellFaces()
    {
        return ncf_;
    }

private:
    // Disabling copy and assigment for now.
    HybridPressureSolver(const HybridPressureSolver&);
    HybridPressureSolver& operator=(const HybridPressureSolver&);

    enum State { Uninitialized, Initialized, Assembled };
    State state_;

    // Solver data.
    fsh_data* data_;
    // Grid.
    GridAdapter grid_;
    // Number of faces per cell.
    std::vector<int> ncf_;
    // B^{-1} storage.
    std::vector<double> Binv_;
    std::vector<double> Binv_mobilityweighted_;
    // Gravity contributions.
    std::vector<double> gpress_;
    std::vector<double> gpress_omegaweighted_;


    FlowBoundaryConditions*
    gather_boundary_conditions(const std::vector<FlowBCTypes>& bctypes ,
                               const std::vector<double>&      bcvalues)
    {
        FlowBoundaryConditions* fbc = flow_conditions_construct(0);

        int ok = fbc != 0;
        std::vector<FlowBCTypes>::size_type i;

        for (i = 0; ok && (i < bctypes.size()); ++i) {
            if (bctypes[ i ] == FBC_PRESSURE) {
                ok = flow_conditions_append(BC_PRESSURE,
                                            static_cast<int>(i),
                                            bcvalues[ i ],
                                            fbc);
            }
            else if (bctypes[ i ] == FBC_FLUX) {
                ok = flow_conditions_append(BC_FLUX_TOTVOL,
                                            static_cast<int>(i),
                                            bcvalues[ i ],
                                            fbc);
            }
        }

        return fbc;
    }


}; // class HybridPressureSolver


#endif // OPM_HYBRIDPRESSURESOLVER_HEADER_INCLUDED
