#_______________________Pmake___________________________________________________
#
#	Makefile : 	Earth Model
# 	MNov 1999	Kris Tingdahl
#_______________________________________________________________________________


set(OD_MODULE_DEPS Geometry )
list ( APPEND OD_NIGHTLY_TEST_PROGS emstoredobjaccess.cc )
set(OD_MODULE_SOURCES
		em2ddatafmtconverter.cc
		embody.cc
		embodyoperator.cc
		embodytr.cc
		emfact.cc
		emfault.cc
		emfault3d.cc
		emfault3dpainter.cc
		emfaultauxdata.cc
		emfaultstickset.cc
		emfaultsticksetflatvieweditor.cc
		emfaultstickpainter.cc
		emfsstofault3d.cc
		emundo.cc
		emhorizon2d.cc
		emhorizon3d.cc
		emhorizonpainter2d.cc
		emhorizonpainter3d.cc
		emhorizonpreload.cc
		emhorizonztransform.cc
		emhorizonztransformer.cc
		emhorizonutils.cc
		emhor2dto3d.cc
		emioobjinfo.cc
		emmanager.cc
		emmarchingcubessurface.cc
		emobject.cc
		emobjectposselector.cc
		emrandomposbody.cc
		empolygonbody.cc
		emposid.cc
		emrandlinegen.cc
		emregion.cc
		emrowcoliterator.cc
		emstoredobjaccess.cc
		emsurface.cc
		emsurfaceauxdata.cc
		emsurfacegeometry.cc
		emsurfaceio.cc
		emsurfaceposprov.cc
		emsurfacetr.cc
		emsurfauxdataio.cc
		faulttrace.cc
		horizongridder.cc
		horizonmerger.cc
		horizonmodifier.cc
		horizonrelation.cc
		horizonscanner.cc
		horizonsorter.cc
		lmkemfaulttransl.cc
		poly2horvol.cc
		initearthmodel.cc
)
OD_INIT_MODULE()
