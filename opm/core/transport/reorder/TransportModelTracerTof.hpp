/*
  Copyright 2012 SINTEF ICT, Applied Mathematics.

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

#ifndef OPM_TRANSPORTMODELTRACERTOF_HEADER_INCLUDED
#define OPM_TRANSPORTMODELTRACERTOF_HEADER_INCLUDED

#include <opm/core/transport/reorder/TransportModelInterface.hpp>
#include <vector>
#include <map>
#include <ostream>
struct UnstructuredGrid;

namespace Opm
{

    class IncompPropertiesInterface;

    /// Implements a first-order finite volume solver for
    /// (single-phase) time-of-flight using reordering.
    /// The equation solved is:
    ///     v \cdot \grad\tau = \phi
    /// where v is the fluid velocity, \tau is time-of-flight and
    /// \phi is the porosity. This is a boundary value problem, where
    /// \tau is specified to be zero on all inflow boundaries.
    class TransportModelTracerTof : public TransportModelInterface
    {
    public:
        /// Construct solver.
        /// \param[in] grid      A 2d or 3d grid.
        /// \param[in] use_multidim_upwind  If true, use multidimensional tof upwinding.
        TransportModelTracerTof(const UnstructuredGrid& grid,
                                const bool use_multidim_upwind = false);

        /// Solve for time-of-flight.
        /// \param[in]  darcyflux         Array of signed face fluxes.
        /// \param[in]  porevolume        Array of pore volumes.
        /// \param[in]  source            Source term. Sign convention is:
        ///                                 (+) inflow flux,
        ///                                 (-) outflow flux.
        /// \param[out] tof               Array of time-of-flight values.
        void solveTof(const double* darcyflux,
                      const double* porevolume,
                      const double* source,
                      std::vector<double>& tof);

        /// Solve for time-of-flight and a number of tracers.
        /// One tracer will be used for each inflow flux specified in
        /// the source parameter.
        /// \param[in]  darcyflux         Array of signed face fluxes.
        /// \param[in]  porevolume        Array of pore volumes.
        /// \param[in]  source            Source term. Sign convention is:
        ///                                 (+) inflow flux,
        ///                                 (-) outflow flux.
        /// \param[out] tof               Array of time-of-flight values (1 per cell).
        /// \param[out] tracer            Array of tracer values (N per cell, where N is
        ///                               the number of cells c for which source[c] > 0.0).
        void solveTofTracer(const double* darcyflux,
                            const double* porevolume,
                            const double* source,
                            std::vector<double>& tof,
                            std::vector<double>& tracer);

    private:
        virtual void solveSingleCell(const int cell);
        void solveSingleCellMultidimUpwind(const int cell);
        virtual void solveMultiCell(const int num_cells, const int* cells);

        void multidimUpwindTerms(const int face, const int upwind_cell,
                                 double& face_term, double& cell_term_factor) const;

    private:
        const UnstructuredGrid& grid_;
        const double* darcyflux_;   // one flux per grid face
        const double* porevolume_;  // one volume per cell
        const double* source_;      // one volumetric source term per cell
        double* tof_;
        double* tracer_;
        int num_tracers_;
        bool use_multidim_upwind_;
        std::vector<double> face_tof_;       // For multidim upwind face tofs.
        mutable std::vector<int> adj_faces_; // For multidim upwind logic.
    };

} // namespace Opm

#endif // OPM_TRANSPORTMODELTRACERTOF_HEADER_INCLUDED
