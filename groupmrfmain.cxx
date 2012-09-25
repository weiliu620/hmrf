#include <common.h>
#include <utility.h>

twister_base_gen_type mygenerator(42u);

unsigned ComputeNumSubs(std::string srcpath);

int BuildGraph(lemon::SmartGraph & theGraph, 
	       lemon::SmartGraph::NodeMap<SuperCoordType> & coordMap,
	       unsigned numSubs, 
	       ImageType3DChar::Pointer maskPtr);

int BuildDataMap(lemon::SmartGraph & theGraph, 
		 lemon::SmartGraph::NodeMap<SuperCoordType> & coordMap,
		 lemon::SmartGraph::NodeMap<vnl_vector<float> > & tsMap,
		 std::string srcpath,
		 ParStruct & par);

int BuildEdgeMap(lemon::SmartGraph & theGraph, 
		 lemon::SmartGraph::EdgeMap<double> & edgeMap,
		 lemon::SmartGraph::NodeMap<SuperCoordType> & coordMap,
		 ParStruct & par);

int InitSubSamples(std::string srcpath,
		   lemon::SmartGraph & theGraph, 
		   lemon::SmartGraph::NodeMap<SuperCoordType> & coordMap,
		   lemon::SmartGraph::NodeMap<unsigned short> & initSubjectMap);

int EstimateMu(lemon::SmartGraph & theGraph, 
	       lemon::SmartGraph::NodeMap<SuperCoordType> & coordMap,
	       lemon::SmartGraph::NodeMap<std::vector<unsigned short> > & cumuSampleMap,
	       lemon::SmartGraph::NodeMap<vnl_vector<float>> & tsMap,
	       ParStruct & par);

int EstimateKappa(lemon::SmartGraph & theGraph, 
		  ParStruct & par);

double EstimatePriorPar(lemon::SmartGraph & theGraph, 
			lemon::SmartGraph::NodeMap<SuperCoordType> & coordMap,
			lemon::SmartGraph::NodeMap<std::vector<unsigned short> > & cumuSampleMap,
			ParStruct & par);

int Sampling(lemon::SmartGraph & theGraph, 
	     lemon::SmartGraph::NodeMap<SuperCoordType> & coordMap,
	     lemon::SmartGraph::NodeMap<std::vector<unsigned short> > & cumuSampleMap,
	     lemon::SmartGraph::NodeMap< boost::dynamic_bitset<> > & rSampleMap,
	     lemon::SmartGraph::EdgeMap<double> & edgeMap,
	     lemon::SmartGraph::NodeMap<vnl_vector<float>> & tsMap,
	     ParStruct & par);

void *SamplingThreads(void * threadArgs);
unsigned short Phi(boost::dynamic_bitset<> xi, boost::dynamic_bitset<> xj);

int CompuTotalEnergy(lemon::SmartGraph & theGraph, 
		     lemon::SmartGraph::NodeMap<SuperCoordType> & coordMap,
		     lemon::SmartGraph::NodeMap<std::vector<unsigned short> > & cumuSampleMap,
		     lemon::SmartGraph::EdgeMap<double> & edgeMap,
		     lemon::SmartGraph::NodeMap<vnl_vector<float>> & tsMap,
		     ParStruct & par);

using namespace lemon;
int main(int argc, char* argv[])
{
     ParStruct par;
     unsigned seed = 0;
     unsigned int emIter = 1;

     std::string dataFile, iLabelFile, oLabelFile, sampleFile;
     std::string fmriPath, initGrplabel, outGrpLabel, outGrpProb, outSubBase, 
	  grpSampleName, subsampledir, initsubpath,  groupprob, subbaseprob;

     bool estprior = false;
     bool initsame = false;

     // program options.
     po::options_description mydesc("Options can only used at commandline");
     mydesc.add_options()
	  ("help,h", "Inference on group mrf.")
	  ("nthreads", po::value<unsigned>(&par.nthreads)->default_value(100),
	   "number of threads to run for sampling")
	  ("burnin,b", po::value<unsigned>(&par.burnin)->default_value(20),
	   "number of scans for burn-in period. ")
	  ("sweepperthread", po::value<unsigned>(&par.sweepPerThread)->default_value(1),
	   "number of scans for burn-in period. ")
	  ("inittemp", po::value<float>(&par.initTemp)->default_value(1),
	   "Initial temperature for annealing.")
	  ("finaltemp", po::value<float>(&par.finalTemp)->default_value(1),
	   "Final temperature for annealing to stop")
	  ("numSamples,n", po::value<unsigned int>(&par.numSamples)->default_value(20),
	   "Number of Monte Carlo samples. ")
	  ("emiter", po::value<unsigned int>(&emIter)->default_value(30),
	   "Number of EM iteration. ")

	  ("alpha", po::value<float>(&par.alpha)->default_value(0.7),
	   "connection between group lebel and individual subject level.")

	  ("beta", po::value<float>(&par.beta)->default_value(0.5),
	   "pairwise interation term")

	  ("gamma", po::value<float>(&par.gamma)->default_value(1),
	   " weights of lilelihood term.")

	  ("alpha0", po::value<float>(&par.alpha0)->default_value(0.7),
	   "Mean of prior on alpha.")

	  ("sigma2", po::value<float>(&par.sigma2)->default_value(1),
	   "variance of prior on alpha")

	  ("seed", po::value<unsigned int>(&seed)->default_value(0),
	   "Random number generator seed.")

	  ("numClusters,k", po::value<unsigned>(&par.numClusters)->default_value(6),
	   "Number of clusters. Default is 6.")

	  ("estprior", po::bool_switch(&estprior), 
	   "whether to estimate alpha and beta, the prior parameter. Default no.")
	  ("initsame", po::bool_switch(&initsame), 
	   "whether to init subject same to group label. Default is no.")

	   ("initgrouplabel,i", po::value<std::string>(&initGrplabel)->default_value("grouplabel.nii"), 
	    "Initial group level label map (Intensity value 1-K. Also used as mask file.")
	   ("initsubpath,t", po::value<std::string>(&initsubpath)->default_value("subpath"), 
	    "Initial subject label maps path")
	  ("fmripath,f", po::value<std::string>(&fmriPath)->default_value("."), 
	   "noised image file")

	  ("subsampledir", po::value<std::string>(&subsampledir)->default_value("subject"), 
	   "Monte Carlo samples file dir.")
	  ("subsampledir", po::value<std::string>(&subsampledir)->default_value("subject"), 
	   "Monte Carlo samples file dir.")
	  ("grpsamplename", po::value<std::string>(&grpSampleName)->default_value("grpSample.nii.gz"), 
	   "Monte Carlo group label samples file name.")

	  ("verbose,v", po::value<unsigned short>(&par.verbose)->default_value(0), 
	   "verbose level. 0 for minimal output. 3 for most output.");

     po::variables_map vm;        
     po::store(po::parse_command_line(argc, argv, mydesc), vm);
     po::notify(vm);    

     try {
	  if ( (vm.count("help")) | (argc == 1) ) {
	       std::cout << "Usage: gropumrf [options]\n";
	       std::cout << mydesc << "\n";
	       return 0;
	  }
     }
     catch(std::exception& e) {
	  std::cout << e.what() << "\n";
	  return 1;
     }    

     // init random generator.
     mygenerator.seed(static_cast<unsigned int>(seed));

     // read in initial grp label. Also used as mask. 1-based label converted to
     // 0-based label.
     ReaderType3DChar::Pointer grpReader = ReaderType3DChar::New();
     grpReader->SetFileName(initGrplabel);
     AddConstantToImageFilterType::Pointer myfilter = AddConstantToImageFilterType::New();
     myfilter->InPlaceOff();
     myfilter->SetInput( grpReader->GetOutput());
     myfilter->SetConstant(-1);
     myfilter->Update();

     // output label init'd.
     ImageType3DChar::Pointer labelPtr = myfilter->GetOutput();
     save3dcharInc(labelPtr, "initGrpLabelCheck.nii.gz");

     // original 1-based group map only used for mask.
     ImageType3DChar::Pointer maskPtr = grpReader->GetOutput();

     // get number of subjects.
     par.numSubs = ComputeNumSubs(fmriPath);
     printf("number of subjects: %i.\n", par.numSubs);

     // init parameters so BuildDataMap knows what to do.
     par.vmm.sub.resize(par.numSubs);

     // Construct graph.
     lemon::SmartGraph theGraph;
     lemon::SmartGraph::NodeMap<SuperCoordType> coordMap(theGraph); // node --> coordinates.

     BuildGraph(theGraph, coordMap, par.numSubs, maskPtr);

     // Define edge map.
     lemon::SmartGraph::EdgeMap<double> edgeMap(theGraph);
     BuildEdgeMap(theGraph, edgeMap, coordMap, par);

     // Build Data map.
     lemon::SmartGraph::NodeMap<vnl_vector<float>> tsMap(theGraph); // node --> time series.
     BuildDataMap(theGraph, coordMap, tsMap, fmriPath, par);

     // At this point, we know time series length so do the remaining
     // initializaion of par.vmm.
     for (unsigned subIdx = 0; subIdx < par.numSubs; subIdx ++) {
	  par.vmm.sub[subIdx].comp.resize(par.numClusters);
	  for (unsigned clsIdx = 0; clsIdx < par.numClusters; clsIdx ++) {
	       par.vmm.sub[subIdx].comp[clsIdx].mu.set_size(par.tsLength);
	       par.vmm.sub[subIdx].comp[clsIdx].mu = 0;
	       par.vmm.sub[subIdx].comp[clsIdx].meanNorm = 0;
	       par.vmm.sub[subIdx].comp[clsIdx].kappa = 0;
	       par.vmm.sub[subIdx].comp[clsIdx].numPts = 0;
	       par.vmm.sub[subIdx].comp[clsIdx].prop = 0;
	  }
     }

     PrintPar(4, par);

     // define a subject initial label map used for initialization of subject
     // label map.
     lemon::SmartGraph::NodeMap<unsigned short> initSubjectMap(theGraph);     
     InitSubSamples(initsubpath, theGraph, coordMap, initSubjectMap);

     // define cumulative sample map. Each node is assigned a vector. The k'th
     // elements of the vector is the number of samples that falls into cluster
     // k.
     lemon::SmartGraph::NodeMap<std::vector<unsigned short> > cumuSampleMap(theGraph);
     for (SmartGraph::NodeIt nodeIt(theGraph); nodeIt !=INVALID; ++ nodeIt) {
     	  cumuSampleMap[nodeIt].resize(par.numClusters, 0);
     }

     // init the cumuSampleMap just to estimate parameters. The k'th bit is set
     // to the number of samples M, since I suppose the M samples are init'd
     // same to the initial group or subject label map.
     for (SmartGraph::NodeIt nodeIt(theGraph); nodeIt !=INVALID; ++ nodeIt) {
	  if (coordMap[nodeIt].subid < par.numSubs && (!initsame) ) {
	       cumuSampleMap[nodeIt].at(initSubjectMap[nodeIt]) = par.numSamples;
	  }
	  else {
	       // only when not initsame for subjects nodes, we init it to
	       // subjects initial label map.
	       cumuSampleMap[nodeIt].at(labelPtr->GetPixel(coordMap[nodeIt].idx)) = par.numSamples;
	  }
     }

     SaveSubCumuSamples(theGraph, coordMap, cumuSampleMap, maskPtr, par, subsampledir);
     SaveGrpCumuSamples(theGraph, coordMap, cumuSampleMap, maskPtr, par, outGrpLabel);
     
     // define a running sample map and init it with the input group
     // labels. This is used for sampling only. And when the whoel scan is done,
     // this map is saved into the cumuSampleMap.
     lemon::SmartGraph::NodeMap< boost::dynamic_bitset<> > rSampleMap(theGraph);
     for (SmartGraph::NodeIt nodeIt(theGraph); nodeIt !=INVALID; ++ nodeIt) {
	  rSampleMap[nodeIt].resize(par.numClusters, 0);
	  if (coordMap[nodeIt].subid < par.numSubs && (!initsame) ) {
	       rSampleMap[nodeIt].set(labelPtr->GetPixel(coordMap[nodeIt].idx), true);
	  }
	  else {
	       // only when not initsame for subjects nodes, we init it to
	       // subjects initial label map.
	       rSampleMap[nodeIt].set(initSubjectMap[nodeIt], true);
	  }
     }


     // as initial step, esimate mu and kappa.
     EstimateMu(theGraph, coordMap, cumuSampleMap, tsMap, par);
     EstimateKappa(theGraph, par);
     PrintPar(2, par);

     // EM starts here.
     for (unsigned short emIterIdx = 0; emIterIdx < emIter; emIterIdx ++) {     
     	  printf("EM iteration %i begin:\n", emIterIdx + 1);
	  par.temperature = par.initTemp * pow( (par.finalTemp / par.initTemp), float(emIterIdx) / float(emIter) );
	  Sampling(theGraph, coordMap, cumuSampleMap, rSampleMap, edgeMap, tsMap, par);
	  CompuTotalEnergy(theGraph, coordMap, cumuSampleMap, edgeMap, tsMap, par); 
	  SaveSubCumuSamples(theGraph, coordMap, cumuSampleMap, maskPtr, par, subsampledir);
	  
     	  // estimate vMF parameters mu, kappa.
     	  printf("EM iteration %i, parameter estimation begin. \n", emIterIdx + 1);
	  EstimateMu(theGraph, coordMap, cumuSampleMap, tsMap, par);
	  EstimateKappa(theGraph, par);
	  PrintPar(2, par);
	  CompuTotalEnergy(theGraph, coordMap, cumuSampleMap, edgeMap, tsMap, par); 
     } // emIterIdx

     return 0;
}     

unsigned ComputeNumSubs(std::string srcpath)
{
     // prepare for sorting directory entries.
     typedef std::vector<boost::filesystem::path> PathVec;
     PathVec  sortedEntries;
     copy(boost::filesystem::directory_iterator(srcpath), boost::filesystem::directory_iterator(), std::back_inserter(sortedEntries) );
     sort(sortedEntries.begin(), sortedEntries.end() );

     // Get number of subjects.
     unsigned numSubs  = sortedEntries.size();
     return numSubs;
}


int BuildGraph(lemon::SmartGraph & theGraph, 
	       lemon::SmartGraph::NodeMap<SuperCoordType> & coordMap,
	       unsigned numSubs, 
	       ImageType3DChar::Pointer maskPtr)
{
     //mask
     IteratorType3DCharIdx maskIt(maskPtr, maskPtr->GetLargestPossibleRegion() );

     ImageType3DChar::IndexType neiIdx;
     
     // map xyz to graph node id.
     ImageType4Int::IndexType nodeMapIdx;
     nodeMapIdx.Fill(0);
     ImageType3DChar::SizeType maskSize =  maskPtr->GetLargestPossibleRegion().GetSize();
     ImageType4Int::SizeType nodeMapSize;
     nodeMapSize[0] = maskSize[0];
     nodeMapSize[1] = maskSize[1];
     nodeMapSize[2] = maskSize[2];
     nodeMapSize[3] = numSubs + 1;

     ImageType4Int::RegionType nodeMapRegion;
     nodeMapRegion.SetSize(nodeMapSize);
     nodeMapRegion.SetIndex(nodeMapIdx);
     ImageType4Int::Pointer nodeMapPtr = ImageType4Int::New();
     nodeMapPtr->SetRegions(nodeMapRegion);
     nodeMapPtr->Allocate();
     nodeMapPtr->FillBuffer(0);

     
     // Add nodes.

     lemon::SmartGraph::Node curNode, neiNode;

     // fill in subjects nodes.
     for (unsigned short subIdx = 0; subIdx < numSubs; subIdx ++) {
	  for (maskIt.GoToBegin(); !maskIt.IsAtEnd(); ++ maskIt) {
	       if (maskIt.Get() > 0) {
		    curNode = theGraph.addNode();
		    coordMap[curNode].idx = maskIt.GetIndex();
		    coordMap[curNode].subid = subIdx;
		    nodeMapIdx[0] = coordMap[curNode].idx[0];
		    nodeMapIdx[1] = coordMap[curNode].idx[1];
		    nodeMapIdx[2] = coordMap[curNode].idx[2];
		    nodeMapIdx[3] = subIdx;
		    nodeMapPtr->SetPixel(nodeMapIdx, theGraph.id(curNode) );
	       }
	  }
     }

     // Fill in group level nodes.
     for (maskIt.GoToBegin(); !maskIt.IsAtEnd(); ++ maskIt) {
     	  if (maskIt.Get() > 0) {
     	       curNode = theGraph.addNode();
	       coordMap[curNode].idx = maskIt.GetIndex();
	       coordMap[curNode].subid = numSubs;
	       
	       nodeMapIdx[0] = coordMap[curNode].idx[0];
	       nodeMapIdx[1] = coordMap[curNode].idx[1];
	       nodeMapIdx[2] = coordMap[curNode].idx[2];
	       nodeMapIdx[3] = numSubs;
	       nodeMapPtr->SetPixel(nodeMapIdx, theGraph.id(curNode) );
	  }
     }

     printf("BuildGraph(): number of Nodes: %i\n", theGraph.maxNodeId()+1 );

     // Add edges. 

     // Define neighborhood iterator
     MyBoundCondType constCondition;
     constCondition.SetConstant(-1);     
     NeighborhoodIteratorType::RadiusType radius;
     radius.Fill(1);
     NeighborhoodIteratorType maskNeiIt(radius, maskPtr, maskPtr->GetLargestPossibleRegion() );
     maskNeiIt.OverrideBoundaryCondition(&constCondition);
     
     // xplus, xminus, yplus, yminus, zplus, zminus
     std::array<unsigned int, 6 > neiIdxSet = {14, 12, 16, 10, 22, 4}; 
     ImageType3DChar::IndexType maskIdx;
     int curNodeId = 0, neiNodeId = 0;
     // std::array<short, 6>::const_iterator neiIdxIt;
     auto neiIdxIt = neiIdxSet.begin();

     // edge within grp.
     for (maskNeiIt.GoToBegin(); !maskNeiIt.IsAtEnd(); ++ maskNeiIt) {
	  if (maskNeiIt.GetCenterPixel() > 0) {
	       maskIdx = maskNeiIt.GetIndex();
	       nodeMapIdx[0] = maskIdx[0];
	       nodeMapIdx[1] = maskIdx[1];
	       nodeMapIdx[2] = maskIdx[2];
	       nodeMapIdx[3] = numSubs;
	       curNodeId = nodeMapPtr->GetPixel(nodeMapIdx);
	       // curNode is group node.
	       curNode = theGraph.nodeFromId( curNodeId );

	       for (neiIdxIt=neiIdxSet.begin(); neiIdxIt < neiIdxSet.end(); neiIdxIt++) {
		    if (maskNeiIt.GetPixel(*neiIdxIt) > 0) {
			 neiIdx = maskNeiIt.GetIndex(*neiIdxIt);
			 nodeMapIdx[0] = neiIdx[0];
			 nodeMapIdx[1] = neiIdx[1];
			 nodeMapIdx[2] = neiIdx[2];
			 nodeMapIdx[3] = numSubs;
			 neiNodeId = nodeMapPtr->GetPixel(nodeMapIdx);
			 // make sure each edge is added only once.
			 if (neiNodeId > curNodeId) {
			      neiNode = theGraph.nodeFromId( neiNodeId );
			      theGraph.addEdge(curNode, neiNode);
			 } // if
		    } // maskIt
	       } // neiIt
	  } // maskNeiIt > 0
     } // maskNeiIt
     printf("BuildGraph(): After adding edges within group, number of edges: %i\n", theGraph.maxEdgeId() + 1);

     // edge between grp and subjects.
     for (maskNeiIt.GoToBegin(); !maskNeiIt.IsAtEnd(); ++ maskNeiIt) {
	  if (maskNeiIt.GetCenterPixel() > 0) {
	       maskIdx = maskNeiIt.GetIndex();
	       nodeMapIdx[0] = maskIdx[0];
	       nodeMapIdx[1] = maskIdx[1];
	       nodeMapIdx[2] = maskIdx[2];

	       // get current node in group.
	       nodeMapIdx[3] = numSubs;
	       curNodeId = nodeMapPtr->GetPixel(nodeMapIdx);
	       curNode = theGraph.nodeFromId( curNodeId );
	       
	       // get subject node and connect them
	       for (unsigned short subIdx = 0; subIdx < numSubs; subIdx ++) {
		    nodeMapIdx[3] = subIdx;
		    neiNodeId = nodeMapPtr->GetPixel(nodeMapIdx);
		    neiNode = theGraph.nodeFromId( neiNodeId );
		    theGraph.addEdge(curNode, neiNode);
	       } // subIdx
	  } // maskNeiIt > 0
     } // MaskNeiIt

     printf("BuildGraph(): After adding edges btw grp and subs, number of edges: %i\n", theGraph.maxEdgeId() + 1);

     // edge within each subject.
     for (unsigned short subIdx = 0; subIdx < numSubs; subIdx ++) {
	  nodeMapIdx[3] = subIdx;

	  for (maskNeiIt.GoToBegin(); !maskNeiIt.IsAtEnd(); ++ maskNeiIt) {
	       if (maskNeiIt.GetCenterPixel() > 0) {
		    maskIdx = maskNeiIt.GetIndex();
		    nodeMapIdx[0] = maskIdx[0];
		    nodeMapIdx[1] = maskIdx[1];
		    nodeMapIdx[2] = maskIdx[2];
		    curNodeId = nodeMapPtr->GetPixel(nodeMapIdx);
		    // curNode is in subject map.
		    curNode = theGraph.nodeFromId( curNodeId );

		    for (neiIdxIt=neiIdxSet.begin(); neiIdxIt < neiIdxSet.end(); neiIdxIt++) {
			 if (maskNeiIt.GetPixel(*neiIdxIt) > 0) {
			      neiIdx = maskNeiIt.GetIndex(*neiIdxIt);
			      nodeMapIdx[0] = neiIdx[0];
			      nodeMapIdx[1] = neiIdx[1];
			      nodeMapIdx[2] = neiIdx[2];
			      neiNodeId = nodeMapPtr->GetPixel(nodeMapIdx);
			      // make sure each edge is added only once.
			      if (neiNodeId > curNodeId) {
				   neiNode = theGraph.nodeFromId( neiNodeId );
				   theGraph.addEdge(curNode, neiNode);
			      } // if
			 } // maskNeiIt
		    } // neiIdxIt
	       } // maskNeiIt >0 
	  }
     } // subIdx

     printf("BuildGraph(): number of edges: %i\n", theGraph.maxEdgeId() + 1);

     return 0;
}


int BuildEdgeMap(lemon::SmartGraph & theGraph, 
		 lemon::SmartGraph::EdgeMap<double> & edgeMap,
		 lemon::SmartGraph::NodeMap<SuperCoordType> & coordMap,
		 ParStruct & par)
{
     for (SmartGraph::EdgeIt edgeIt(theGraph); edgeIt != INVALID; ++ edgeIt) {
	  if (coordMap[theGraph.u(edgeIt)].subid < par.numSubs && 
	      coordMap[theGraph.v(edgeIt)].subid < par.numSubs) {
	       // within subjects.
	       edgeMap[edgeIt] = par.beta;
	  }
	  else if (coordMap[theGraph.u(edgeIt)].subid == par.numSubs && 
		   coordMap[theGraph.v(edgeIt)].subid == par.numSubs) {
	       // within group.
	       edgeMap[edgeIt] = par.beta;
	  }
	  else {
	       // must be between group and subjects.
	       edgeMap[edgeIt] = par.alpha;
	  }
     }
     return 0;

}
int BuildDataMap(lemon::SmartGraph & theGraph, 
		 lemon::SmartGraph::NodeMap<SuperCoordType> & coordMap,
		 lemon::SmartGraph::NodeMap<vnl_vector<float> > & tsMap,
		 std::string srcpath,
		 ParStruct & par)
{
     // some of the code below refers to boost filesystem tutorial tut04.cpp.
     // prepare for sorting directory entries.
     typedef std::vector<boost::filesystem::path> PathVec;
     PathVec  sortedEntries;
     copy(boost::filesystem::directory_iterator(srcpath), boost::filesystem::directory_iterator(), std::back_inserter(sortedEntries) );
     sort(sortedEntries.begin(), sortedEntries.end() );

     // Get number of subjects.
     unsigned numSubs  = sortedEntries.size();

     // init file reader pointers.
     std::vector<ReaderType4DFloat::Pointer> readerVec(numSubs);
     std::vector<ImageType4DF::Pointer> srcPtrVec(numSubs);
     for (unsigned subIdx = 0; subIdx < numSubs; subIdx ++) {
	  readerVec[subIdx] = ReaderType4DFloat::New();
	  readerVec[subIdx]->SetFileName( sortedEntries[subIdx].string() );
	  readerVec[subIdx]->Update();
	  srcPtrVec[subIdx]= readerVec[subIdx]->GetOutput();

	  // also save file name into par.
	  boost::filesystem::path thispath = sortedEntries[subIdx];
	  if(thispath.extension().string().compare(".gz") == 0 ) {
	       // must be a .nii.gz file. Remove .gz suffix so the filename is
	       // abc.nii.
	       thispath = thispath.stem();
	  }
	  par.vmm.sub[subIdx].name.assign(thispath.filename().stem().string() );
     }

     ImageType4DF::SizeType srcSize = readerVec[0]->GetOutput()->GetLargestPossibleRegion().GetSize();
     par.tsLength = srcSize[3];

     printf("BuildDataMap(): Time series length: %i.\n", par.tsLength);

     SuperCoordType superCoord;
     ImageType4DF::IndexType srcIdx;
     for (SmartGraph::NodeIt nodeIt(theGraph); nodeIt !=INVALID; ++ nodeIt) {
     	  superCoord = coordMap[nodeIt];
	  // Only when not in group map, do this step, because only subject map
	  // has data term, while group nodes do not.
	  if (superCoord.subid < numSubs) {
	       tsMap[nodeIt].set_size(par.tsLength);
	       srcIdx[0] = superCoord.idx[0];
	       srcIdx[1] = superCoord.idx[1];
	       srcIdx[2] = superCoord.idx[2];
	       for (srcIdx[3] = 0; srcIdx[3] < par.tsLength; srcIdx[3] ++) {
		    tsMap[nodeIt][srcIdx[3]] = srcPtrVec[superCoord.subid]->GetPixel(srcIdx);
	       }
	  }
     }

     return 0;
}


/* Read initial subject label map from files. */
int InitSubSamples(std::string srcpath,
		   lemon::SmartGraph & theGraph, 
		   lemon::SmartGraph::NodeMap<SuperCoordType> & coordMap,
		   lemon::SmartGraph::NodeMap<unsigned short> & initSubjectMap)
{
     // some of the code below refers to boost filesystem tutorial tut04.cpp.
     // prepare for sorting directory entries.
     typedef std::vector<boost::filesystem::path> PathVec;
     PathVec  sortedEntries;
     copy(boost::filesystem::directory_iterator(srcpath), boost::filesystem::directory_iterator(), std::back_inserter(sortedEntries) );
     sort(sortedEntries.begin(), sortedEntries.end() );
     unsigned numSubs  = sortedEntries.size();

     printf("InitSubSamples(): numSubs = %i\n", numSubs);

     // init file reader pointers.
     std::vector<ReaderType3DChar::Pointer> readerVec(numSubs);
     std::vector<ImageType3DChar::Pointer> srcPtrVec(numSubs);
     for (unsigned subIdx = 0; subIdx < numSubs; subIdx ++) {
	  readerVec[subIdx] = ReaderType3DChar::New();
	  readerVec[subIdx]->SetFileName( sortedEntries[subIdx].string() );
	  readerVec[subIdx]->Update();
	  srcPtrVec[subIdx]= readerVec[subIdx]->GetOutput();
     }

     SuperCoordType superCoord;
     for (SmartGraph::NodeIt nodeIt(theGraph); nodeIt !=INVALID; ++ nodeIt) {
     	  superCoord = coordMap[nodeIt];
	  // when not in group map, do this step, since this function only read
	  // in subject's initial label map.
	  if (superCoord.subid < numSubs) {
	       initSubjectMap[nodeIt] = srcPtrVec[superCoord.subid]->GetPixel(superCoord.idx) - 1;
	  }
     }
     return 0;
}


int EstimateMu(lemon::SmartGraph & theGraph, 
	       lemon::SmartGraph::NodeMap<SuperCoordType> & coordMap,
	       lemon::SmartGraph::NodeMap<std::vector<unsigned short> > & cumuSampleMap,
	       lemon::SmartGraph::NodeMap<vnl_vector<float>> & tsMap,
	       ParStruct & par)
{

     // reset all mu and numPts to zero.
     for (unsigned subIdx = 0; subIdx < par.numSubs; subIdx ++) {
	  for (unsigned clsIdx = 0; clsIdx < par.numClusters; clsIdx ++) {
	       par.vmm.sub[subIdx].comp[clsIdx].mu = 0;
	       par.vmm.sub[subIdx].comp[clsIdx].numPts = 0;
	  }
     }

     SuperCoordType superCoord;
     for (SmartGraph::NodeIt nodeIt(theGraph); nodeIt !=INVALID; ++ nodeIt) {
	  superCoord = coordMap[nodeIt];
	  // if this is a subject node.
	  if (superCoord.subid < par.numSubs) {
	       for (unsigned clsIdx = 0; clsIdx < par.numClusters; clsIdx ++) {
		    par.vmm.sub[superCoord.subid].comp[clsIdx].mu += tsMap[nodeIt] * cumuSampleMap[nodeIt][clsIdx];
		    par.vmm.sub[superCoord.subid].comp[clsIdx].numPts += cumuSampleMap[nodeIt][clsIdx];
	       }
	  } // subid < par.numSubs
     }

     for (unsigned subIdx = 0; subIdx < par.numSubs; subIdx ++) {
	  for (unsigned clsIdx = 0; clsIdx < par.numClusters; clsIdx ++) {     
	       if(par.vmm.sub[subIdx].comp[clsIdx].numPts > 0) {
		    // printf("numPts: %d. ", par.vmm.sub[subIdx].comp[clsIdx].numPts);
		    // printVnlVector(par.vmm.sub[subIdx].comp[clsIdx].mu, 5);
		    par.vmm.sub[subIdx].comp[clsIdx].meanNorm = par.vmm.sub[subIdx].comp[clsIdx].mu.two_norm() / par.vmm.sub[subIdx].comp[clsIdx].numPts;
		    par.vmm.sub[subIdx].comp[clsIdx].mu.normalize();
		    // printVnlVector(par.vmm.sub[subIdx].comp[clsIdx].mu, 5);
	       }
	       else {
		    par.vmm.sub[subIdx].comp[clsIdx].meanNorm = 0;
	       }
	  }
     }
	       
     printf("EstimateMu(). Done.\n");
     return 0;
}

int EstimateKappa(lemon::SmartGraph & theGraph, 
		  ParStruct & par)
{
     float kappa = 0, kappa_new = 0;
     double Ap = 0;
     float RBar = 0;
     float Dim = par.tsLength;

     for (unsigned subIdx = 0; subIdx < par.numSubs; subIdx ++) {
	  for (unsigned clsIdx = 0; clsIdx < par.numClusters; clsIdx ++) {
	       if (par.vmm.sub[subIdx].comp[clsIdx].numPts > 0){

		    RBar = par.vmm.sub[subIdx].comp[clsIdx].meanNorm;
		    kappa_new = RBar * (Dim - RBar * RBar) / (1 - RBar * RBar);
		    if (kappa_new  == 0 ) {
			 par.vmm.sub[subIdx].comp[clsIdx].kappa = kappa_new;
			 continue;
		    }
	       
		    unsigned iter = 0;
		    do {
			 iter ++;
			 kappa = kappa_new;
			 Ap = exp(logBesselI(Dim/2, kappa) - logBesselI(Dim/2 - 1, kappa));
			 kappa_new = kappa - ( (Ap - RBar) / (1 - Ap * Ap - (Dim - 1) * Ap / kappa)  );
			 if (par.verbose >= 3) {
			      printf("    sub[%i] cls[%i] kappa: %3.1f -> %3.1f\n", subIdx + 1, clsIdx + 1, kappa, kappa_new);
			 }
		    }
		    while(vnl_math_abs(kappa_new - kappa) > 0.01 * kappa && iter < 5);
		    par.vmm.sub[subIdx].comp[clsIdx].kappa = kappa_new;
	       } // numPts > 0
	       else {
		    par.vmm.sub[subIdx].comp[clsIdx].kappa = 0;
	       }

	  } // clsIdx
     } // subIdx

     printf("EstimateKappa(). Done.\n");
     return 0;
}

double EstimatePriorPar(lemon::SmartGraph & theGraph, 
			lemon::SmartGraph::NodeMap<SuperCoordType> & coordMap,
			lemon::SmartGraph::NodeMap<std::vector<unsigned short> > & cumuSampleMap,
			ParStruct & par)
{

     return 0;
}

int Sampling(lemon::SmartGraph & theGraph, 
	     lemon::SmartGraph::NodeMap<SuperCoordType> & coordMap,
	     lemon::SmartGraph::NodeMap<std::vector<unsigned short> > & cumuSampleMap,
	     lemon::SmartGraph::NodeMap< boost::dynamic_bitset<> > & rSampleMap,
	     lemon::SmartGraph::EdgeMap<double> & edgeMap,
	     lemon::SmartGraph::NodeMap<vnl_vector<float>> & tsMap,
	     ParStruct & par)
{

     unsigned taskid = 0;
     pthread_t thread_ids[par.nthreads];
     ThreadArgs threadArgs[par.nthreads];

     // reset cumuSampleMap to zero.
     for (SmartGraph::NodeIt nodeIt(theGraph); nodeIt !=INVALID; ++ nodeIt) {
	  cumuSampleMap[nodeIt].assign(par.numClusters, 0);
     }

     // Compute the number of nodes that each thread computes.
     unsigned totalNumNodes = theGraph.maxNodeId()+1;     
     unsigned numNodesPerThread = ceil( double(totalNumNodes) / double(par.nthreads) );

     // compute normalization constant of von Mises
     // Fisher. vmfLogConst[i] is the log (c_d (kappa) ) for the i'th
     // clusters. See page 1350 of "Clustering on the Unit-Sphere
     // using von Mises Fisher..."
     std::vector< vnl_vector<double> >vmfLogConst(par.numSubs);
     float myD = par.tsLength;
     double const Pi = 4 * atan(1);
     unsigned subIdx = 0, clsIdx = 0;
     for (subIdx = 0; subIdx < par.numSubs; subIdx ++) {
	  vmfLogConst[subIdx].set_size(par.numClusters);
	  for (clsIdx = 0; clsIdx < par.numClusters; clsIdx ++) {
	       if (par.vmm.sub[subIdx].comp[clsIdx].kappa > 1) {
		    vmfLogConst[subIdx][clsIdx] = (myD/2 - 1) * log (par.vmm.sub[subIdx].comp[clsIdx].kappa) - myD/2 * log(2*Pi) -  logBesselI(myD/2 - 1, par.vmm.sub[subIdx].comp[clsIdx].kappa);
	       }
	       else {
		    vmfLogConst[subIdx][clsIdx] = 25.5; // the limit when kappa -> 0
	       }
	  } // for clsIdx
     } // for subIdx

     for (unsigned taskid = 0; taskid < par.nthreads; taskid ++) {
	  threadArgs[taskid].taskid = taskid;
	  threadArgs[taskid].theGraphPtr = & theGraph;
	  threadArgs[taskid].coordMapPtr = & coordMap;
	  threadArgs[taskid].edgeMapPtr = & edgeMap;
	  threadArgs[taskid].rSampleMapPtr = & rSampleMap;
	  threadArgs[taskid].tsMapPtr = & tsMap;
	  threadArgs[taskid].parPtr = & par;
	  threadArgs[taskid].numThreads = par.nthreads;
	  threadArgs[taskid].startNodeid = taskid * numNodesPerThread;
	  threadArgs[taskid].vmfLogConstPtr = & vmfLogConst;
	  
	  // make sure the last thread have correct node id to process.
	  if (taskid == par.nthreads - 1) {
	       threadArgs[taskid].endNodeid = totalNumNodes - 1;
	  }
	  else {
	       threadArgs[taskid].endNodeid = (taskid + 1) * numNodesPerThread -1;
	  }
     }

     // sampling starts here.
     printf("Sampling() starts scan...\n");
     for (unsigned scanIdx = 0; scanIdx < par.burnin + par.numSamples; scanIdx ++) {

	  printf("%i, ", scanIdx+1);
	  fflush(stdout);
	  for (taskid = 0; taskid < par.nthreads; taskid ++) {
	       pthread_create(&thread_ids[taskid], NULL, SamplingThreads, (void*) &threadArgs[taskid]);
	  }

	  for (taskid = 0; taskid < par.nthreads; taskid ++) {
	       pthread_join(thread_ids[taskid], NULL);
	  }

	  // after burnin pariod, save it to correct place.
	  if (scanIdx >= par.burnin) {
	       for (SmartGraph::NodeIt nodeIt(theGraph); nodeIt !=INVALID; ++ nodeIt) {
		    cumuSampleMap[nodeIt][rSampleMap[nodeIt].find_first()] ++;
	       }
	  }
     } // scanIdx
	    
     printf("Sampling done.\n");
     return 0;
}


void *SamplingThreads(void * threadArgs)
{
     ThreadArgs * args = (ThreadArgs *) threadArgs;
     unsigned taskid = args->taskid;
     lemon::SmartGraph * theGraphPtr = args->theGraphPtr;
     lemon::SmartGraph::NodeMap<SuperCoordType> * coordMapPtr = args->coordMapPtr;
     lemon::SmartGraph::NodeMap< boost::dynamic_bitset<> > * rSampleMapPtr = args->rSampleMapPtr;
     lemon::SmartGraph::EdgeMap<double> * edgeMapPtr = args->edgeMapPtr;
     lemon::SmartGraph::NodeMap<vnl_vector<float>> * tsMapPtr = args->tsMapPtr;
     std::vector< vnl_vector<double> > * vmfLogConstPtr = args->vmfLogConstPtr;
     ParStruct * parPtr = args->parPtr;
     unsigned numThreads = args->numThreads;



     // define random generator.
     boost::random::mt19937 gen;
     boost::random::uniform_int_distribution<> roll_die(0, (*parPtr).numClusters - 1);

     boost::uniform_real<> uni_dist(0,1); // uniform distribution.
     boost::variate_generator<boost::mt19937&, boost::uniform_real<> > uni(gen, uni_dist);

     // Sampling starts here.
     unsigned sweepIdx = 0;
     float oldPriorEngy = 0, newPriorEngy = 0, oldDataEngy = 0, newDataEngy = 0, denergy;
     boost::dynamic_bitset<> cand;
     cand.resize((*parPtr).numClusters);
     float p_acpt = 0;
     // cl is current label, and nl is new label. s is subject id.
     unsigned short cl = 0, nl = 0, s = 0;
     lemon::SmartGraph::Node curNode;

     for (sweepIdx = 0 ;sweepIdx < parPtr->sweepPerThread; sweepIdx ++) {
	  for (unsigned nodeId = args->startNodeid; nodeId <= args->endNodeid; nodeId++) {
	       curNode = (*theGraphPtr).nodeFromId(nodeId);

	       // compute old and new prior energy.
	       oldPriorEngy = 0;
	       newPriorEngy = 0;
	       cand.reset();
	       cand[roll_die(gen)] = 1;
	       for (SmartGraph::IncEdgeIt edgeIt(*theGraphPtr, curNode); edgeIt != INVALID; ++ edgeIt) {
	  	    oldPriorEngy += (*edgeMapPtr)[edgeIt] * Phi( (*rSampleMapPtr)[(*theGraphPtr).u(edgeIt)], (*rSampleMapPtr)[(*theGraphPtr).v(edgeIt)]);
	  	    newPriorEngy += (*edgeMapPtr)[edgeIt] * Phi(cand, (*rSampleMapPtr)[(*theGraphPtr).runningNode(edgeIt)]);
	       }
	       denergy = newPriorEngy - oldPriorEngy;

	       // if current node is in subject, compute data term. Otherwise,
	       // the current node must be in group. Then do not compute data
	       // term.
	       if ( (*coordMapPtr)[curNode].subid < (*parPtr).numSubs) {
		    cl = (*rSampleMapPtr)[curNode].find_first(); // current label.
		    nl = cand.find_first(); // new label.
		    s = (*coordMapPtr)[curNode].subid;
		    oldDataEngy = - (*vmfLogConstPtr)[s][cl] - (*parPtr).vmm.sub[s].comp[cl].kappa * inner_product((*tsMapPtr)[curNode], (*parPtr).vmm.sub[s].comp[cl].mu);
		    newDataEngy = - (*vmfLogConstPtr)[s][nl] - (*parPtr).vmm.sub[s].comp[nl].kappa * inner_product((*tsMapPtr)[curNode], (*parPtr).vmm.sub[s].comp[nl].mu);
		    denergy = denergy + (newDataEngy - oldDataEngy);
	       }
	       denergy = denergy / (*parPtr).temperature;

	       if (denergy <= 0) {
		    (*rSampleMapPtr)[curNode] = cand;
	       }
	       else {
		    p_acpt = exp(-denergy);
		    if (uni() < p_acpt) {
			 (*rSampleMapPtr)[curNode] = cand;
		    }
	       } // else
	  } // curNode
     } // sweepIdx
}

unsigned short Phi(boost::dynamic_bitset<> xi, boost::dynamic_bitset<> xj)
{
     return ((xi == xj) ? 0:1);
}

int CompuTotalEnergy(lemon::SmartGraph & theGraph, 
		     lemon::SmartGraph::NodeMap<SuperCoordType> & coordMap,
		     lemon::SmartGraph::NodeMap<std::vector<unsigned short> > & cumuSampleMap,
		     lemon::SmartGraph::EdgeMap<double> & edgeMap,
		     lemon::SmartGraph::NodeMap<vnl_vector<float>> & tsMap,
		     ParStruct & par)
{
     double totalPriorEng = 0, totalDataEng = 0;
     double samplePriorEng = 0, sampleDataEng = 0;
     boost::dynamic_bitset<> curBit(par.numClusters), runningBit(par.numClusters);
     double b = 0, bcur = 0, M0 = 0;
     SuperCoordType superCoord;
     for (SmartGraph::NodeIt nodeIt(theGraph); nodeIt !=INVALID; ++ nodeIt) {
	  for (unsigned clsIdx = 0; clsIdx < par.numClusters; clsIdx ++) {
	       // check if clsIdx has some samples that fall into it. If there
	       // are n samples, just compute one sample's energy and times n.
	       if (cumuSampleMap[nodeIt][clsIdx] > 0) {
		    curBit.reset(), curBit[clsIdx] = 1;
		    M0 = 0;
		    for (runningBit.reset(), runningBit[0]=1; !runningBit.test(par.numClusters-1); runningBit<<=1) {
			 b = 0;
			 superCoord = coordMap[nodeIt];
			 for (SmartGraph::IncEdgeIt edgeIt(theGraph, nodeIt); edgeIt != INVALID; ++ edgeIt) {
			      superCoord = coordMap[theGraph.runningNode(edgeIt)];
			      b = b - edgeMap[edgeIt] * Phi(runningBit, curBit) ;
			 } // incEdgeIt
			 M0 += exp (b);

			 if (runningBit == curBit) {
			      bcur = b;
			 }
		    } // runningBit.
		    samplePriorEng = (bcur - log(M0));
		    totalPriorEng += cumuSampleMap[nodeIt][clsIdx] * samplePriorEng;
	       } // cumuSampleMap > 0
	  } // clsIdx
     } // nodeIt

     totalPriorEng = totalPriorEng / par.numSamples;

     // convert log likelihood to energy.
     totalPriorEng = - totalPriorEng;

     // for data energy.
     std::vector< vnl_vector<double> >vmfLogConst(par.numSubs);
     float myD = par.tsLength;
     double const Pi = 4 * atan(1);
     unsigned subIdx = 0, clsIdx = 0;
     for (subIdx = 0; subIdx < par.numSubs; subIdx ++) {
	  vmfLogConst[subIdx].set_size(par.numClusters);
	  for (clsIdx = 0; clsIdx < par.numClusters; clsIdx ++) {
	       if (par.vmm.sub[subIdx].comp[clsIdx].kappa > 1) {
		    vmfLogConst[subIdx][clsIdx] = (myD/2 - 1) * log (par.vmm.sub[subIdx].comp[clsIdx].kappa) - myD/2 * log(2*Pi) -  logBesselI(myD/2 - 1, par.vmm.sub[subIdx].comp[clsIdx].kappa);
	       }
	       else {
		    vmfLogConst[subIdx][clsIdx] = 25.5; // the limit when kappa -> 0
	       }
	  }
     } // for subIdx.

     
     // cl is current label, s is subject id.
     unsigned short s = 0;
     for (SmartGraph::NodeIt nodeIt(theGraph); nodeIt !=INVALID; ++ nodeIt) {
	  if (coordMap[nodeIt].subid < par.numSubs) {
	       for (unsigned clsIdx = 0; clsIdx < par.numClusters; clsIdx ++) {
		    // check if clsIdx has some samples that fall into it. If there
		    // are n samples, just compute one sample's energy and times n.
		    if (cumuSampleMap[nodeIt][clsIdx] > 0) {
			 s = coordMap[nodeIt].subid;
			 sampleDataEng = vmfLogConst[s][clsIdx] + par.vmm.sub[s].comp[clsIdx].kappa * inner_product(tsMap[nodeIt], par.vmm.sub[s].comp[clsIdx].mu);
			 totalDataEng += cumuSampleMap[nodeIt][clsIdx] * sampleDataEng;
			 if (totalDataEng != totalDataEng) {
			      printf("NAN happend at sub: %d, [%d %d %d]\n", s, coordMap[nodeIt].idx[0], coordMap[nodeIt].idx[1], coordMap[nodeIt].idx[2]);
			      exit(1);
			 }
		    } 
	       }// clsIdx
	  } // subid < numSubs
     } // nodeIt

     totalDataEng = totalDataEng / par.numSamples;

     // convert log likelihood to energy.
     totalDataEng = - totalDataEng;

     printf("totalPriorEng: %E, totalDataEng: %E, total energy ( E[log P(X,Y)] ): %E.\n", totalPriorEng, totalDataEng, totalPriorEng + totalDataEng);
     
     return 0;
}


