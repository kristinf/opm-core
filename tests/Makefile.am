AM_CPPFLAGS =					\
-I$(top_srcdir)					\
$(OPM_BOOST_CPPFLAGS) $(SUPERLU_CPPFLAGS)	\
$(ERT_CPPFLAGS)

AM_LDFLAGS = $(OPM_BOOST_LDFLAGS) $(ERT_LDFLAGS)


LDADD =	$(top_builddir)/lib/libopmcore.la


noinst_PROGRAMS =				\
bo_resprop_test					\
monotcubicinterpolator_test			\
param_test					\
pvt_test					\
relperm_test                 			\
sparsetable_test				\
sparsevector_test				\
test_cartgrid					\
test_column_extract				\
test_dgbasis					\
test_lapack					\
test_quadratures				\
test_read_grid					\
test_read_vag					\
test_readpolymer				\
test_sf2p					\
test_velocityinterpolation			\
test_wachspresscoord				\
test_wells					\
test_writeVtkData				\
unit_test


bo_resprop_test_SOURCES = bo_resprop_test.cpp

monotcubicinterpolator_test_SOURCES = monotcubicinterpolator_test.cpp

param_test_SOURCES        = param_test.cpp
param_test_LDADD          = $(LDADD) $(BOOST_UNIT_TEST_FRAMEWORK_LIB)

pvt_test_SOURCES          = pvt_test.cpp

relperm_test_SOURCES      = relperm_test.cpp

sparsetable_test_SOURCES  = sparsetable_test.cpp
sparsetable_test_LDADD    = $(LDADD) $(BOOST_UNIT_TEST_FRAMEWORK_LIB)

sparsevector_test_SOURCES = sparsevector_test.cpp
sparsevector_test_LDADD   = $(LDADD) $(BOOST_UNIT_TEST_FRAMEWORK_LIB)

test_cartgrid_SOURCES     = test_cartgrid.cpp

test_column_extract_SOURCES = test_column_extract.cpp
test_column_extract_LDADD = $(LDADD) $(BOOST_UNIT_TEST_FRAMEWORK_LIB)

test_dgbasis_SOURCES = test_dgbasis.cpp
test_dgbasis_LDADD = $(LDADD) $(BOOST_UNIT_TEST_FRAMEWORK_LIB)

test_lapack_SOURCES       = test_lapack.cpp
test_lapack_LDADD         = $(LAPACK_LIBS) $(BLAS_LIBS) $(LIBS)

test_quadratures_SOURCES = test_quadratures.cpp
test_quadratures_LDADD   = $(LDADD) $(BOOST_UNIT_TEST_FRAMEWORK_LIB)

test_readpolymer_SOURCES  = test_readpolymer.cpp
test_read_vag_SOURCES     = test_read_vag.cpp

test_sf2p_SOURCES         = test_sf2p.cpp

test_velocityinterpolation_SOURCES = test_velocityinterpolation.cpp
test_velocityinterpolation_LDADD   = $(LDADD) $(BOOST_UNIT_TEST_FRAMEWORK_LIB)

test_wachspresscoord_SOURCES = test_wachspresscoord.cpp
test_wachspresscoord_LDADD   = $(LDADD) $(BOOST_UNIT_TEST_FRAMEWORK_LIB)

test_writeVtkData_SOURCES = test_writeVtkData.cpp

test_wells_SOURCES        = test_wells.cpp
test_wells_LDADD          = $(LDADD) $(BOOST_UNIT_TEST_FRAMEWORK_LIB)

unit_test_SOURCES         = unit_test.cpp


if UMFPACK
noinst_PROGRAMS    += test_cfs_tpfa
noinst_PROGRAMS    += test_jacsys
test_jacsys_SOURCES = test_jacsys.cpp
endif


if BUILD_AGMG
noinst_PROGRAMS   += test_agmg
test_agmg_SOURCES  = test_agmg.cpp
endif


if HAVE_ERT
noinst_PROGRAMS   += test_ert
test_ert_SOURCES   = test_ert.cpp
endif
