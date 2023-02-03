#include <Hadrons/Application.hpp>
#include <Hadrons/Modules.hpp>

using namespace Grid;
using namespace Hadrons;

int main(int argc, char *argv[])
{
    // parse command line //////////////////////////////////////////////////////
    std::string parameterFileName;
    
    if (argc < 2)
    {
        std::cerr << "usage: " << argv[0] << " <parameter file> [Grid options]";
        std::cerr << std::endl;
        std::exit(EXIT_FAILURE);
    }
    parameterFileName = argv[1];
    
    // initialise Grid /////////////////////////////////////////////////////////
    Grid_init(&argc, &argv);
    
    // initialise application //////////////////////////////////////////////////
    Application            application;
    Application::GlobalPar globalPar;
    
    // reading parameters
    {
        XmlReader reader(parameterFileName);

        read(reader, "global", globalPar);

        // read other application-specific parameters here
    }

    // global initialisation
    application.setPar(globalPar);

    // create modules //////////////////////////////////////////////////////////

    // add modules here with application.createModule<...>(...)
    // the one below is just an example
    MIO::LoadNersc::Par loadPar;
    loadPar.file = "./cnfg/ckpoint_lat";
    application.createModule<MIO::LoadNersc>("gauge", loadPar);

    MGauge::FundtoAdjoint::Par adjPar;
    adjPar.gaugeconf = "gauge";
    application.createModule<MGauge::FundtoAdjoint>("adjgauge", adjPar);
  
    MSource::Z2Adj::Par z2wallPar;
    z2wallPar.tA = 0;
    z2wallPar.tB = 0;
    application.createModule<MSource::Z2Adj>("z2wall", z2wallPar);

    MSink::ScalarPoint::Par sinkPar;
    sinkPar.mom = "0 0 0";
    application.createModule<MSink::ScalarPoint>("sink", sinkPar);

    MAction::MobiusDWFAdj::Par actionPar;
    actionPar.gauge = "adjgauge";
    actionPar.Ls = 8;
    actionPar.mass = 0.05;
    actionPar.M5 = 1.8;
    actionPar.b = 1.0;
    actionPar.c = 0.0;
    actionPar.boundary = "1 1 1 -1";
    actionPar.twist = "0. 0. 0. 0.";
    application.createModule<MAction::MobiusDWFAdj>("mobiusadj", actionPar);

    MSolver::RBPrecCGAdj::Par solverPar;
    solverPar.action = "mobiusadj";
    solverPar.residual = 1.0e-8;
    solverPar.maxIteration = 10000;
    application.createModule<MSolver::RBPrecCGAdj>("cg", solverPar);

    MFermion::GaugePropAdj::Par fermPar;
    fermPar.solver = "cg";
    fermPar.source = "z2wall";
    application.createModule<MFermion::GaugePropAdj>("prop", fermPar);

    MContraction::MesonAdj::Par mesPar;
    mesPar.output = "mesons/pt_ll";
    mesPar.q1 = "prop";
    mesPar.q2 = "prop";
    mesPar.gammas = "all";
    mesPar.sink = "sink";
    application.createModule<MContraction::MesonAdj>("meson_pt_ll", mesPar);
    
    // execution ///////////////////////////////////////////////////////////////
    try
    {
        application.run();
    }
    catch (const std::exception& e)
    {
        Exceptions::abort(e);
    }
    
    // epilogue ////////////////////////////////////////////////////////////////
    LOG(Message) << "Grid is finalizing now" << std::endl;
    Grid_finalize();
    
    return EXIT_SUCCESS;
}
