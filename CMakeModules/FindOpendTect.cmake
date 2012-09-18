INCLUDE ( ${OpendTect_DIR}/CMakeModules/OD_SetupOD.cmake )
LINK_DIRECTORIES ( ${OpendTect_DIR}/${OD_EXEC_OUTPUT_RELPATH} )
SET ( OD_CORE_MODULE_NAMES_od Basic;Algo;General;Database;Strat;Network;Batch;Usage;Geometry;EarthModel;MMProc;Seis;NLA;AttributeEngine;Velocity;VolumeProcessing;PreStackProcessing;Attributes;EMAttrib;MPEEngine;Well;WellAttrib;uiBase;uiTools;uiCmdDriver;uiFlatView;uiIo;uiSysAdm;uiNLA;uiSeis;uiStrat;uiEarthModel;uiWell;uiVelocity;uiVolumeProcessing;uiPreStackProcessing;uiAttributes;uiEMAttrib;uiMPE;uiViewer2D;uiWellAttrib;SoOD;visBase;visSurvey;uiCoin;uiVis;uiODMain;AllNonUi )
SET( OD_Basic_INCLUDEPATH ${OpendTect_DIR}/include/Basic )
SET( OD_Algo_DEPS Basic )
SET( OD_Algo_INCLUDEPATH ${OpendTect_DIR}/include/Algo )
SET( OD_General_DEPS Algo )
SET( OD_General_INCLUDEPATH ${OpendTect_DIR}/include/General )
SET( OD_Database_DEPS General )
SET( OD_Database_INCLUDEPATH ${OpendTect_DIR}/include/Database )
SET( OD_Strat_DEPS General )
SET( OD_Strat_INCLUDEPATH ${OpendTect_DIR}/include/Strat )
SET( OD_Network_DEPS General )
SET( OD_Network_INCLUDEPATH ${OpendTect_DIR}/include/Network )
SET( OD_Batch_DEPS Network )
SET( OD_Batch_INCLUDEPATH ${OpendTect_DIR}/include/Batch )
SET( OD_Usage_DEPS General )
SET( OD_Usage_INCLUDEPATH ${OpendTect_DIR}/include/Usage )
SET( OD_Geometry_DEPS General )
SET( OD_Geometry_INCLUDEPATH ${OpendTect_DIR}/include/Geometry )
SET( OD_EarthModel_DEPS Geometry )
SET( OD_EarthModel_INCLUDEPATH ${OpendTect_DIR}/include/EarthModel )
SET( OD_MMProc_DEPS Network )
SET( OD_MMProc_INCLUDEPATH ${OpendTect_DIR}/include/MMProc )
SET( OD_Seis_DEPS Geometry;MMProc )
SET( OD_Seis_INCLUDEPATH ${OpendTect_DIR}/include/Seis )
SET( OD_NLA_DEPS General )
SET( OD_NLA_INCLUDEPATH ${OpendTect_DIR}/include/NLA )
SET( OD_AttributeEngine_DEPS NLA;Seis )
SET( OD_AttributeEngine_INCLUDEPATH ${OpendTect_DIR}/include/AttributeEngine )
SET( OD_Velocity_DEPS AttributeEngine;EarthModel )
SET( OD_Velocity_INCLUDEPATH ${OpendTect_DIR}/include/Velocity )
SET( OD_VolumeProcessing_DEPS Velocity )
SET( OD_VolumeProcessing_INCLUDEPATH ${OpendTect_DIR}/include/VolumeProcessing )
SET( OD_PreStackProcessing_DEPS Velocity )
SET( OD_PreStackProcessing_INCLUDEPATH ${OpendTect_DIR}/include/PreStackProcessing )
SET( OD_Attributes_DEPS AttributeEngine;PreStackProcessing )
SET( OD_Attributes_INCLUDEPATH ${OpendTect_DIR}/include/Attributes )
SET( OD_EMAttrib_DEPS EarthModel;AttributeEngine )
SET( OD_EMAttrib_INCLUDEPATH ${OpendTect_DIR}/include/EMAttrib )
SET( OD_MPEEngine_DEPS EarthModel;AttributeEngine )
SET( OD_MPEEngine_INCLUDEPATH ${OpendTect_DIR}/include/MPEEngine )
SET( OD_Well_DEPS General )
SET( OD_Well_INCLUDEPATH ${OpendTect_DIR}/include/Well )
SET( OD_WellAttrib_DEPS Well;AttributeEngine;PreStackProcessing;Strat )
SET( OD_WellAttrib_INCLUDEPATH ${OpendTect_DIR}/include/WellAttrib )
SET( OD_uiBase_DEPS General )
SET( OD_uiBase_INCLUDEPATH ${OpendTect_DIR}/include/uiBase )
SET( OD_uiTools_DEPS uiBase;Network )
SET( OD_uiTools_INCLUDEPATH ${OpendTect_DIR}/include/uiTools )
SET( OD_uiCmdDriver_DEPS uiTools )
SET( OD_uiCmdDriver_INCLUDEPATH ${OpendTect_DIR}/include/uiCmdDriver )
SET( OD_uiFlatView_DEPS uiTools )
SET( OD_uiFlatView_INCLUDEPATH ${OpendTect_DIR}/include/uiFlatView )
SET( OD_uiIo_DEPS uiTools;MMProc;uiFlatView;Geometry )
SET( OD_uiIo_INCLUDEPATH ${OpendTect_DIR}/include/uiIo )
SET( OD_uiSysAdm_DEPS uiIo )
SET( OD_uiSysAdm_INCLUDEPATH ${OpendTect_DIR}/include/uiSysAdm )
SET( OD_uiNLA_DEPS uiIo;NLA;Well;Seis )
SET( OD_uiNLA_INCLUDEPATH ${OpendTect_DIR}/include/uiNLA )
SET( OD_uiSeis_DEPS uiIo;Seis;Usage;Velocity )
SET( OD_uiSeis_INCLUDEPATH ${OpendTect_DIR}/include/uiSeis )
SET( OD_uiStrat_DEPS uiIo;Strat )
SET( OD_uiStrat_INCLUDEPATH ${OpendTect_DIR}/include/uiStrat )
SET( OD_uiEarthModel_DEPS uiStrat;EarthModel )
SET( OD_uiEarthModel_INCLUDEPATH ${OpendTect_DIR}/include/uiEarthModel )
SET( OD_uiWell_DEPS uiStrat;Well )
SET( OD_uiWell_INCLUDEPATH ${OpendTect_DIR}/include/uiWell )
SET( OD_uiVelocity_DEPS uiSeis;Velocity )
SET( OD_uiVelocity_INCLUDEPATH ${OpendTect_DIR}/include/uiVelocity )
SET( OD_uiVolumeProcessing_DEPS uiVelocity;VolumeProcessing )
SET( OD_uiVolumeProcessing_INCLUDEPATH ${OpendTect_DIR}/include/uiVolumeProcessing )
SET( OD_uiPreStackProcessing_DEPS uiSeis;PreStackProcessing )
SET( OD_uiPreStackProcessing_INCLUDEPATH ${OpendTect_DIR}/include/uiPreStackProcessing )
SET( OD_uiAttributes_DEPS uiPreStackProcessing;uiVolumeProcessing;Attributes )
SET( OD_uiAttributes_INCLUDEPATH ${OpendTect_DIR}/include/uiAttributes )
SET( OD_uiEMAttrib_DEPS uiAttributes;uiEarthModel;EMAttrib )
SET( OD_uiEMAttrib_INCLUDEPATH ${OpendTect_DIR}/include/uiEMAttrib )
SET( OD_uiMPE_DEPS uiAttributes;MPEEngine )
SET( OD_uiMPE_INCLUDEPATH ${OpendTect_DIR}/include/uiMPE )
SET( OD_uiViewer2D_DEPS uiMPE )
SET( OD_uiViewer2D_INCLUDEPATH ${OpendTect_DIR}/include/uiViewer2D )
SET( OD_uiWellAttrib_DEPS uiWell;uiAttributes;WellAttrib )
SET( OD_uiWellAttrib_INCLUDEPATH ${OpendTect_DIR}/include/uiWellAttrib )
SET( OD_SoOD_INCLUDEPATH ${OpendTect_DIR}/include/SoOD )
SET( OD_visBase_DEPS SoOD;Geometry )
SET( OD_visBase_INCLUDEPATH ${OpendTect_DIR}/include/visBase )
SET( OD_visSurvey_DEPS visBase;EarthModel;MPEEngine;Well )
SET( OD_visSurvey_INCLUDEPATH ${OpendTect_DIR}/include/visSurvey )
SET( OD_uiCoin_DEPS visSurvey;uiTools )
SET( OD_uiCoin_INCLUDEPATH ${OpendTect_DIR}/include/uiCoin )
SET( OD_uiVis_DEPS uiCoin;uiMPE )
SET( OD_uiVis_INCLUDEPATH ${OpendTect_DIR}/include/uiVis )
SET( OD_uiODMain_DEPS uiVis;uiAttributes;uiNLA;uiWellAttrib;uiEMAttrib;uiViewer2D )
SET( OD_uiODMain_INCLUDEPATH ${OpendTect_DIR}/include/uiODMain )
SET( OD_Annotations_DEPS uiODMain )
SET( OD_Annotations_INCLUDEPATH ${OpendTect_DIR}/plugins/Annotations )
SET( OD_Bouncy_DEPS uiODMain )
SET( OD_Bouncy_INCLUDEPATH ${OpendTect_DIR}/plugins/Bouncy )
SET( OD_ExpAttribs_DEPS AttributeEngine )
SET( OD_ExpAttribs_INCLUDEPATH ${OpendTect_DIR}/plugins/ExpAttribs )
SET( OD_GapDecon_DEPS Attributes )
SET( OD_GapDecon_INCLUDEPATH ${OpendTect_DIR}/plugins/GapDecon )
SET( OD_GoogleTranslate_DEPS uiODMain )
SET( OD_GoogleTranslate_INCLUDEPATH ${OpendTect_DIR}/plugins/GoogleTranslate )
SET( OD_GMT_DEPS EarthModel;Seis;Well )
SET( OD_GMT_INCLUDEPATH ${OpendTect_DIR}/plugins/GMT )
SET( OD_Hello_DEPS Basic )
SET( OD_Hello_INCLUDEPATH ${OpendTect_DIR}/plugins/Hello )
SET( OD_HorizonAttrib_DEPS Basic;Algo;AttributeEngine;PreStackProcessing;Attributes;EarthModel;MPEEngine )
SET( OD_HorizonAttrib_INCLUDEPATH ${OpendTect_DIR}/plugins/HorizonAttrib )
SET( OD_Madagascar_DEPS AttributeEngine )
SET( OD_Madagascar_INCLUDEPATH ${OpendTect_DIR}/plugins/Madagascar )
SET( OD_MadagascarAttribs_DEPS Attributes )
SET( OD_MadagascarAttribs_INCLUDEPATH ${OpendTect_DIR}/plugins/MadagascarAttribs )
SET( OD_VoxelConnectivityFilter_DEPS VolumeProcessing )
SET( OD_VoxelConnectivityFilter_INCLUDEPATH ${OpendTect_DIR}/plugins/VoxelConnectivityFilter )
SET( OD_uiBouncy_DEPS Bouncy;uiODMain )
SET( OD_uiBouncy_INCLUDEPATH ${OpendTect_DIR}/plugins/uiBouncy )
SET( OD_uiDPSDemo_DEPS uiODMain )
SET( OD_uiDPSDemo_INCLUDEPATH ${OpendTect_DIR}/plugins/uiDPSDemo )
SET( OD_uiExpAttribs_DEPS ExpAttribs;uiODMain )
SET( OD_uiExpAttribs_INCLUDEPATH ${OpendTect_DIR}/plugins/uiExpAttribs )
SET( OD_uiGapDecon_DEPS uiAttributes;uiFlatView;GapDecon )
SET( OD_uiGapDecon_INCLUDEPATH ${OpendTect_DIR}/plugins/uiGapDecon )
SET( OD_uiGMT_DEPS uiODMain;GMT )
SET( OD_uiGMT_INCLUDEPATH ${OpendTect_DIR}/plugins/uiGMT )
SET( OD_uiGoogleIO_DEPS uiODMain )
SET( OD_uiGoogleIO_INCLUDEPATH ${OpendTect_DIR}/plugins/uiGoogleIO )
SET( OD_uiGrav_DEPS uiODMain )
SET( OD_uiGrav_INCLUDEPATH ${OpendTect_DIR}/plugins/uiGrav )
SET( OD_uiHello_DEPS uiODMain )
SET( OD_uiHello_INCLUDEPATH ${OpendTect_DIR}/plugins/uiHello )
SET( OD_uiHorizonAttrib_DEPS uiODMain;HorizonAttrib )
SET( OD_uiHorizonAttrib_INCLUDEPATH ${OpendTect_DIR}/plugins/uiHorizonAttrib )
SET( OD_uiImpGPR_DEPS uiODMain )
SET( OD_uiImpGPR_INCLUDEPATH ${OpendTect_DIR}/plugins/uiImpGPR )
SET( OD_uiMadagascar_DEPS uiODMain;Madagascar )
SET( OD_uiMadagascar_INCLUDEPATH ${OpendTect_DIR}/plugins/uiMadagascar )
SET( OD_uiMadagascarAttribs_DEPS uiAttributes;MadagascarAttribs )
SET( OD_uiMadagascarAttribs_INCLUDEPATH ${OpendTect_DIR}/plugins/uiMadagascarAttribs )
SET( OD_uiTutMadagascar_DEPS Seis;uiFlatView;uiODMain;Madagascar )
SET( OD_uiTutMadagascar_INCLUDEPATH ${OpendTect_DIR}/plugins/uiTutMadagascar )
SET( OD_uiVoxelConnectivityFilter_DEPS uiODMain;VoxelConnectivityFilter )
SET( OD_uiVoxelConnectivityFilter_INCLUDEPATH ${OpendTect_DIR}/plugins/uiVoxelConnectivityFilter )
SET( OD_uiPreStackViewer_DEPS uiODMain )
SET( OD_uiPreStackViewer_INCLUDEPATH ${OpendTect_DIR}/plugins/uiPreStackViewer )
SET( OD_CmdDriver_DEPS uiCmdDriver;uiODMain )
SET( OD_CmdDriver_INCLUDEPATH ${OpendTect_DIR}/plugins/CmdDriver )
