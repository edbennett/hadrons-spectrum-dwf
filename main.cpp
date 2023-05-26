#include <Hadrons/Application.hpp>
#include <Hadrons/Modules.hpp>

using namespace Grid;
using namespace Hadrons;

struct SpectrumPar: Serializable
{
  GRID_SERIALIZABLE_CLASS_MEMBERS(SpectrumPar,
				  int, Ls,
				  double, mass,
				  double, M5,
				  double, b,
				  double, c,
				  std::string, cfg_prefix);
};

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
    SpectrumPar spectrumPar;
    
    // reading parameters
    {
        XmlReader reader(parameterFileName);

        read(reader, "global", globalPar);
	read(reader, "spectrum", spectrumPar);
        // read other application-specific parameters here
    }

    // global initialisation
    application.setPar(globalPar);

    // create modules //////////////////////////////////////////////////////////

    MIO::LoadNersc::Par loadPar;
    loadPar.file = spectrumPar.cfg_prefix;
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
    actionPar.Ls = spectrumPar.Ls;
    actionPar.mass = spectrumPar.mass;
    actionPar.M5 = spectrumPar.M5;
    actionPar.b = spectrumPar.b;
    actionPar.c = spectrumPar.c;
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

    MContraction::WardIdentity::Par mresPar;
    mresPar.output = "mesons/mres";
    mresPar.action = "mobiusadj";
    mresPar.source = "z2wall";
    mresPar.prop = "prop_5d";
    mresPar.mass = spectrumPar.mass;
    application.createModule<MContraction::WardIdentityAdj>("mres", mresPar);
    
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
