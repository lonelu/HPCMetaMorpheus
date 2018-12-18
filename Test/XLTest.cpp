﻿#include "XLTest.h"
#include "../EngineLayer/CommonParameters.h"
#include "../TaskLayer/XLSearchTask/XLSearchParameters.h"
#include "../TaskLayer/MetaMorpheusTask.h"
#include "../TaskLayer/XLSearchTask/TaskLayer.XLSearchTask.h"
#include "../TaskLayer/DbForTask.h"
#include "../TaskLayer/EverythingRunnerEngine.h"
#include "../EngineLayer/Ms2ScanWithSpecificMass.h"

using namespace Chemistry;
using namespace EngineLayer;
using namespace EngineLayer::CrosslinkSearch;
using namespace EngineLayer::Indexing;
using namespace MassSpectrometry;
using namespace MzLibUtil;
using namespace Nett;
using namespace NUnit::Framework;
using namespace Proteomics;
using namespace Proteomics::AminoAcidPolymer;
using namespace Proteomics::Fragmentation;
using namespace Proteomics::ProteolyticDigestion;
using namespace TaskLayer;
using namespace UsefulProteomicsDatabases;

namespace Test
{

IndexingResults *XLTest::privateindexResults;
CommonParameters *XLTest::privatecommonParameters;
XlSearchParameters *XLTest::privatexlSearchParameters;
std::vector<Protein*> XLTest::privateproteinList;
std::vector<Modification*> XLTest::privatevariableModifications;
std::vector<Modification*> XLTest::privatefixedModifications;
Crosslinker *XLTest::privatecrosslinker;
std::vector<PeptideWithSetModifications*> XLTest::privatedigestedList;

	IndexingResults *XLTest::getindexResults()
	{
		return privateindexResults;
	}

	void XLTest::setindexResults(IndexingResults *value)
	{
		privateindexResults = value;
	}

	CommonParameters *XLTest::getcommonParameters()
	{
		return privatecommonParameters;
	}

	void XLTest::setcommonParameters(CommonParameters *value)
	{
		privatecommonParameters = value;
	}

	XlSearchParameters *XLTest::getxlSearchParameters()
	{
		return privatexlSearchParameters;
	}

	void XLTest::setxlSearchParameters(XlSearchParameters *value)
	{
		privatexlSearchParameters = value;
	}

	std::vector<Protein*> XLTest::getproteinList()
	{
		return privateproteinList;
	}

	void XLTest::setproteinList(const std::vector<Protein*> &value)
	{
		privateproteinList = value;
	}

	std::vector<Modification*> XLTest::getvariableModifications()
	{
		return privatevariableModifications;
	}

	void XLTest::setvariableModifications(const std::vector<Modification*> &value)
	{
		privatevariableModifications = value;
	}

	std::vector<Modification*> XLTest::getfixedModifications()
	{
		return privatefixedModifications;
	}

	void XLTest::setfixedModifications(const std::vector<Modification*> &value)
	{
		privatefixedModifications = value;
	}

	Crosslinker *XLTest::getcrosslinker()
	{
		return privatecrosslinker;
	}

	void XLTest::setcrosslinker(Crosslinker *value)
	{
		privatecrosslinker = value;
	}

	std::vector<PeptideWithSetModifications*> XLTest::getdigestedList()
	{
		return privatedigestedList;
	}

	void XLTest::setdigestedList(const std::vector<PeptideWithSetModifications*> &value)
	{
		privatedigestedList = value;
	}

	void XLTest::XlTestXlPosCal()
	{
		auto prot = new Protein(L"MNNNKQQQQ", nullptr);
		std::vector<DigestionMotif*> motifs = {new DigestionMotif(L"K", nullptr, 1, nullptr)};
		Protease *protease = new Protease(L"New Custom Protease", CleavageSpecificity::Full, nullptr, nullptr, motifs);
		ProteaseDictionary::Dictionary->Add(protease->Name, protease);
		DigestionParams *digestionParams = new DigestionParams(protease: protease->Name, minPeptideLength: 1, initiatorMethionineBehavior: InitiatorMethionineBehavior::Retain);
		std::vector<Modification*> variableModifications;

		auto ye = prot->Digest(digestionParams, std::vector<Modification*>(), variableModifications).ToList();

		auto pep = ye[0];
		Assert::AreEqual(pep.BaseSequence, L"MNNNK");
		Crosslinker *crosslinker = new Crosslinker();
		crosslinker->SelectCrosslinker(CrosslinkerType::DSS);
		Assert::AreEqual(crosslinker->getCrosslinkerModSites(), L"K");
		Assert::AreEqual(Residue::GetResidue(crosslinker->getCrosslinkerModSites()).MonoisotopicMass, 128.09496301518999, 1e-9);
		auto n = pep.Fragment(DissociationType::HCD, FragmentationTerminus::N);
		auto c = pep.Fragment(DissociationType::HCD, FragmentationTerminus::C);
		Assert::AreEqual(n->Count(), 4);
		Assert::AreEqual(c->Count(), 4);
		Assert::AreEqual(c->First().NeutralMass, 146.10552769899999, 1e-6);
		auto x = CrosslinkSpectralMatch::GetPossibleCrosslinkerModSites(crosslinker->getCrosslinkerModSites().ToCharArray(), pep).ToArray();
		Assert::AreEqual(x[0], 5);

		auto pep2 = ye[2];
		Assert::AreEqual(L"MNNNKQQQQ", pep2.BaseSequence);
		auto n2 = pep2.Fragment(DissociationType::HCD, FragmentationTerminus::N);
		auto c2 = pep2.Fragment(DissociationType::HCD, FragmentationTerminus::C);
		Assert::AreEqual(n2->Count(), 8);
		Assert::AreEqual(c2->Count(), 8);
		auto x2 = CrosslinkSpectralMatch::GetPossibleCrosslinkerModSites(crosslinker->getCrosslinkerModSites().ToCharArray(), pep2).ToArray();
		Assert::AreEqual(x2[0], 5);

		//Test crosslinker with multiple types of mod
		auto protSTC = new Protein(L"GASTACK", nullptr);
		auto peps = protSTC->Digest(digestionParams, std::vector<Modification*>(), variableModifications).ToList();
		auto pepSTC = peps[0];
		Assert::AreEqual(pepSTC.BaseSequence, L"GASTACK");
		Crosslinker *crosslinker2 = new Crosslinker(L"ST", L"C", L"crosslinkerSTC", false, -18.01056, 0, 0, 0, 0, 0, 0);
		std::wstring crosslinkerModSitesAll = std::wstring((crosslinker2->getCrosslinkerModSites() + crosslinker2->getCrosslinkerModSites2()).ToCharArray().Distinct()->ToArray());
		Assert::AreEqual(crosslinkerModSitesAll, L"STC");

		delete crosslinker2;
		delete protSTC;
		delete crosslinker;
//C# TO C++ CONVERTER TODO TASK: A 'delete digestionParams' statement was not added since digestionParams was passed to a method or constructor. Handle memory management manually.
//C# TO C++ CONVERTER TODO TASK: A 'delete protease' statement was not added since protease was passed to a method or constructor. Handle memory management manually.
		delete prot;
	}

	void XLTest::XlTestGenerateIntensityRanks()
	{
		std::vector<double> intensity = {1.1, 1.1, 0.5, 3.2, 0.5, 6.0};
		std::vector<int> rank = CrosslinkSpectralMatch::GenerateIntensityRanks(intensity);
		std::vector<int> Rank = {4, 3, 6, 2, 5, 1};
		Assert::AreEqual(rank, Rank);
	}

	void XLTest::XlTest_BSA_DSSO()
	{
		//Generate parameters
		PpmTolerance tempVar(10);
		DigestionParams tempVar2(minPeptideLength: 5);
		auto commonParameters = new CommonParameters(, DissociationType::EThcD, false, , , , , , , 1, , , , , , , , &tempVar, , , &tempVar2);

		auto xlSearchParameters = new XlSearchParameters();

		//Create databases contain two protein.
		auto proteinList = std::vector<Protein*>
		{
			new Protein(L"EKVLTSSAR", L"Fake01"),
			new Protein(L"LSQKFPK", L"Fake02")
		};

		ModificationMotif motif1;
		ModificationMotif::TryGetMotif(L"M", motif1);
		Modification *mod1 = new Modification(_originalId: L"Oxidation of M", _modificationType: L"Common Variable", _target: motif1, _locationRestriction: L"Anywhere.", _monoisotopicMass: 15.99491461957);
		ModificationMotif motif2;
		ModificationMotif::TryGetMotif(L"C", motif2);
		Modification *mod2 = new Modification(_originalId: L"Carbamidomethyl of C", _modificationType: L"Common Fixed", _target: motif2, _locationRestriction: L"Anywhere.", _monoisotopicMass: 57.02146372068994);
		auto variableModifications = std::vector<Modification*> {mod1};
		auto fixedModifications = std::vector<Modification*> {mod2};
		auto localizeableModifications = std::vector<Modification*>();

		//Run index engine
		auto indexEngine = new IndexingEngine(proteinList, variableModifications, fixedModifications, 1, DecoyType::Reverse, commonParameters, 30000, false, std::vector<FileInfo*>(), std::vector<std::wstring>());

		auto indexResults = static_cast<IndexingResults*>(indexEngine->Run());

		auto indexedFragments = indexResults->getFragmentIndex().Where([&] (std::any p)
		{
		delete indexEngine;
		delete mod2;
		delete mod1;
		delete xlSearchParameters;
//C# TO C++ CONVERTER TODO TASK: A 'delete commonParameters' statement was not added since commonParameters was passed to a method or constructor. Handle memory management manually.
			return p != nullptr;
		}).SelectMany([&] (std::any v)
		{
		delete indexEngine;
		delete mod2;
		delete mod1;
		delete xlSearchParameters;
//C# TO C++ CONVERTER TODO TASK: A 'delete commonParameters' statement was not added since commonParameters was passed to a method or constructor. Handle memory management manually.
			return v;
		}).ToList();
		Assert::AreEqual(82, indexedFragments.size());
		Assert::AreEqual(3, indexResults->getPeptideIndex().size());

		//Get MS2 scans.
		auto myMsDataFile = new XLTestDataFile();
		CommonParameters tempVar3();
		auto listOfSortedms2Scans = MetaMorpheusTask::GetMs2Scans(myMsDataFile, L"", &tempVar3).OrderBy([&] (std::any b)
		{
			b::PrecursorMass;
		})->ToArray();

		//Generate crosslinker, which is DSSO here.
		Crosslinker tempVar4();
		Crosslinker *crosslinker = (&tempVar4)->SelectCrosslinker(CrosslinkerType::DSSO);

		std::vector<CrosslinkSpectralMatch*> possiblePsms(listOfSortedms2Scans.size());
		CrosslinkSearchEngine tempVar5(possiblePsms, listOfSortedms2Scans, indexResults->getPeptideIndex(), indexResults->getFragmentIndex(), 0, commonParameters, crosslinker, xlSearchParameters->getRestrictToTopNHits(), xlSearchParameters->getCrosslinkSearchTopNum(), xlSearchParameters->getXlQuench_H2O(), xlSearchParameters->getXlQuench_NH2(), xlSearchParameters->getXlQuench_Tris(), new std::vector<std::wstring> { });
		(&tempVar5)->Run();

		auto newPsms = possiblePsms.Where([&] (std::any p)
		{
//C# TO C++ CONVERTER TODO TASK: A 'delete myMsDataFile' statement was not added since myMsDataFile was passed to a method or constructor. Handle memory management manually.
		delete indexEngine;
		delete mod2;
		delete mod1;
		delete xlSearchParameters;
//C# TO C++ CONVERTER TODO TASK: A 'delete commonParameters' statement was not added since commonParameters was passed to a method or constructor. Handle memory management manually.
			return p != nullptr;
		}).ToList();
		for (auto item : newPsms)
		{
			item.SetFdrValues(0, 0, 0, 0, 0, 0, 0, 0, 0, false);
		}

		//Test newPsms
		Assert::AreEqual(3, newPsms.size());

		//Test Output
		auto task = new XLSearchTask();
		task->WritePepXML_xl(newPsms, proteinList, L"", variableModifications, fixedModifications, nullptr, TestContext::CurrentContext->TestDirectory, L"pep.XML", std::vector<std::wstring> { });

		//Test PsmCross.XlCalculateTotalProductMasses
		//var psmCrossAlpha = new CrosslinkSpectralMatch(digestedList[1], 0, 0, 0, listOfSortedms2Scans[0], commonParameters.DigestionParams, new List<MatchedFragmentIon>());
		//var psmCrossBeta = new CrosslinkSpectralMatch(digestedList[2], 0, 0, 0, listOfSortedms2Scans[0], commonParameters.DigestionParams, new List<MatchedFragmentIon>());
		//var linkPos = CrosslinkSpectralMatch.GetPossibleCrosslinkerModSites(crosslinker.CrosslinkerModSites.ToCharArray(), digestedList[1]);
		//var productMassesAlphaList = CrosslinkedPeptide.XlGetTheoreticalFragments(DissociationType.EThcD, false, crosslinker, linkPos, digestedList[2].MonoisotopicMass, digestedList[1]);
		//Assert.AreEqual(productMassesAlphaList.First().Value.Count, 50); //TO DO: The number here should be manually verified.
		File::Delete(LR"(singlePsms.tsv)");
		File::Delete(LR"(pep.XML.pep.xml)");
		File::Delete(LR"(allPsms.tsv)");

		delete task;
//C# TO C++ CONVERTER TODO TASK: A 'delete myMsDataFile' statement was not added since myMsDataFile was passed to a method or constructor. Handle memory management manually.
		delete indexEngine;
		delete mod2;
		delete mod1;
		delete xlSearchParameters;
//C# TO C++ CONVERTER TODO TASK: A 'delete commonParameters' statement was not added since commonParameters was passed to a method or constructor. Handle memory management manually.
	}

	void XLTest::XlTest_BSA_DSS_file()
	{
		auto task = Toml::ReadFile<XLSearchTask*>(FileSystem::combine(TestContext::CurrentContext->TestDirectory, LR"(XlTestData/XLSearchTaskconfig_BSA_DSS_23747.toml)"), MetaMorpheusTask::tomlConfig);
		FileSystem::createDirectory(FileSystem::combine(Environment::CurrentDirectory, LR"(TESTXlTestData)"));
		DbForTask *db = new DbForTask(FileSystem::combine(TestContext::CurrentContext->TestDirectory, LR"(XlTestData/BSA.fasta)"), false);
		std::wstring raw = FileSystem::combine(TestContext::CurrentContext->TestDirectory, LR"(XlTestData/BSA_DSS_23747.mzML)");
		EverythingRunnerEngine tempVar({(L"Task", task)}, {raw}, {db}, FileSystem::combine(Environment::CurrentDirectory, LR"(TESTXlTestData)"));
		(&tempVar)->Run();
		Directory::Delete(FileSystem::combine(Environment::CurrentDirectory, LR"(TESTXlTestData)"), true);

//C# TO C++ CONVERTER TODO TASK: A 'delete db' statement was not added since db was passed to a method or constructor. Handle memory management manually.
	}

	void XLTest::XlTest_GenerateUserDefinedCrosslinker()
	{
		XlSearchParameters *xlSearchParameters = new XlSearchParameters();
		xlSearchParameters->setCrosslinkerType(CrosslinkerType::UserDefined);
		xlSearchParameters->setCrosslinkerName(L"CrossST-C");
		xlSearchParameters->setCrosslinkerResidues(L"ST");
		xlSearchParameters->setCrosslinkerResidues2(L"C");
		xlSearchParameters->setCrosslinkerTotalMass(std::make_optional(-18.01056));
		auto crosslinker = XLSearchTask::GenerateUserDefinedCrosslinker(xlSearchParameters);

//C# TO C++ CONVERTER TODO TASK: A 'delete xlSearchParameters' statement was not added since xlSearchParameters was passed to a method or constructor. Handle memory management manually.
	}

	void XLTest::XlTest_DiffCrosslinkSites()
	{
		//Generate parameters
		DigestionParams tempVar(minPeptideLength: 4);
		auto commonParameters = new CommonParameters(, , false, , , , , , , 1, , , , , , , , , , , &tempVar);

		auto xlSearchParameters = new XlSearchParameters();
		xlSearchParameters->setCrosslinkerType(CrosslinkerType::UserDefined);
		xlSearchParameters->setCrosslinkerName(L"CrossST-C");
		xlSearchParameters->setCrosslinkerResidues(L"ST");
		xlSearchParameters->setCrosslinkerResidues2(L"C");
		xlSearchParameters->setCrosslinkerTotalMass(std::make_optional(-18.01056));

		//Create databases contain two protein.
		auto proteinList = std::vector<Protein*>
		{
			new Protein(L"VLTAR", L"Fake01"),
			new Protein(L"LCQK", L"Fake02")
		};

		ModificationMotif motif1;
		ModificationMotif::TryGetMotif(L"M", motif1);
		Modification *mod1 = new Modification(_originalId: L"Oxidation of M", _modificationType: L"Common Variable", _target: motif1, _locationRestriction: L"Anywhere.", _monoisotopicMass: 15.99491461957);
		auto variableModifications = std::vector<Modification*> {mod1};
		auto fixedModifications = std::vector<Modification*>();
		auto localizeableModifications = std::vector<Modification*>();

		std::unordered_map<Modification*, unsigned short> modsDictionary;

		int i = 1;
		for (auto mod : variableModifications)
		{
			modsDictionary.emplace(mod, static_cast<unsigned short>(i));
			i++;
		}
		for (auto mod : localizeableModifications)
		{
			modsDictionary.emplace(mod, static_cast<unsigned short>(i));
			i++;
		}

		//Generate digested peptide lists.
		std::vector<PeptideWithSetModifications*> digestedList;
		for (auto item : proteinList)
		{
			auto digested = item->Digest(commonParameters->getDigestionParams(), fixedModifications, variableModifications).ToList();
			digestedList.insert(digestedList.end(), digested.begin(), digested.end());
		}

		//Run index engine
		auto indexEngine = new IndexingEngine(proteinList, variableModifications, fixedModifications, 1, DecoyType::Reverse, commonParameters, 30000, false, std::vector<FileInfo*>(), std::vector<std::wstring>());

		auto indexResults = static_cast<IndexingResults*>(indexEngine->Run());

		//Get MS2 scans.
		auto myMsDataFile = new XLTestDataFileDiffSite();
		CommonParameters tempVar2();
		auto listOfSortedms2Scans = MetaMorpheusTask::GetMs2Scans(myMsDataFile, L"", &tempVar2).OrderBy([&] (std::any b)
		{
			b::PrecursorMass;
		})->ToArray();

		//Generate crosslinker, which is UserDefined here.
		auto crosslinker = XLSearchTask::GenerateUserDefinedCrosslinker(xlSearchParameters);

		//TwoPassCrosslinkSearchEngine.Run().
		std::vector<CrosslinkSpectralMatch*> possiblePsms(listOfSortedms2Scans.size());
		CrosslinkSearchEngine tempVar3(possiblePsms, listOfSortedms2Scans, indexResults->getPeptideIndex(), indexResults->getFragmentIndex(), 0, commonParameters, crosslinker, xlSearchParameters->getRestrictToTopNHits(), xlSearchParameters->getCrosslinkSearchTopNum(), xlSearchParameters->getXlQuench_H2O(), xlSearchParameters->getXlQuench_NH2(), xlSearchParameters->getXlQuench_Tris(), new std::vector<std::wstring> { });
		(&tempVar3)->Run();

		auto newPsms = possiblePsms.Where([&] (std::any p)
		{
//C# TO C++ CONVERTER TODO TASK: A 'delete myMsDataFile' statement was not added since myMsDataFile was passed to a method or constructor. Handle memory management manually.
		delete indexEngine;
		delete mod1;
//C# TO C++ CONVERTER TODO TASK: A 'delete xlSearchParameters' statement was not added since xlSearchParameters was passed to a method or constructor. Handle memory management manually.
//C# TO C++ CONVERTER TODO TASK: A 'delete commonParameters' statement was not added since commonParameters was passed to a method or constructor. Handle memory management manually.
			return p != nullptr;
		}).ToList();
		Assert::AreEqual(1, newPsms.size());

//C# TO C++ CONVERTER TODO TASK: A 'delete myMsDataFile' statement was not added since myMsDataFile was passed to a method or constructor. Handle memory management manually.
		delete indexEngine;
		delete mod1;
//C# TO C++ CONVERTER TODO TASK: A 'delete xlSearchParameters' statement was not added since xlSearchParameters was passed to a method or constructor. Handle memory management manually.
//C# TO C++ CONVERTER TODO TASK: A 'delete commonParameters' statement was not added since commonParameters was passed to a method or constructor. Handle memory management manually.
	}

	void XLTest::CrosslinkCreateTest()
	{
		XlSearchParameters tempVar();
		Assert::That((XLSearchTask::GenerateUserDefinedCrosslinker(&tempVar))->GetType()->Equals(Crosslinker::typeid));
	}

	void XLTest::DeadendPeptideTest()
	{
		std::wstring myFileXl = FileSystem::combine(TestContext::CurrentContext->TestDirectory, LR"(XlTestData\BSA_DSSO_ETchD6010.mgf)");
		std::wstring myDatabaseXl = FileSystem::combine(TestContext::CurrentContext->TestDirectory, LR"(XlTestData\BSA.fasta)");
		std::wstring outputFolder = FileSystem::combine(TestContext::CurrentContext->TestDirectory, LR"(TestDeadendPeptide)");

		XLSearchTask *xLSearchTask = new XLSearchTask();
		CommonParameters tempVar(, = DissociationType::HCD, = true, = true, = 3, = 12, = true, = false, = 1, = 5, = 200, = 0.01, = false, = true, = false, = false, = nullptr, new PpmTolerance(51000));
		xLSearchTask->setCommonParameters(&tempVar);

		XLSearchTask *xLSearchTask2 = new XLSearchTask();
		CommonParameters tempVar2(, = DissociationType::HCD, = true, = true, = 3, = 12, = true, = false, = 1, = 5, = 200, = 0.01, = false, = true, = false, = false, = nullptr, new PpmTolerance(112000));
		xLSearchTask2->setCommonParameters(&tempVar2);
		XlSearchParameters tempVar3();
		xLSearchTask2->setXlSearchParameters(&tempVar3);
		xLSearchTask2->getXlSearchParameters()->setXlQuench_Tris(false);
		xLSearchTask2->getXlSearchParameters()->setXlQuench_H2O(false);
		xLSearchTask2->getXlSearchParameters()->setXlQuench_NH2(true);
		FileSystem::createDirectory(outputFolder);

		xLSearchTask->RunTask(outputFolder, {new DbForTask(myDatabaseXl, false)}, {myFileXl}, L"test");
		xLSearchTask2->RunTask(outputFolder, {new DbForTask(myDatabaseXl, false)}, {myFileXl}, L"test");
		Directory::Delete(outputFolder, true);
		Directory::Delete(FileSystem::combine(TestContext::CurrentContext->TestDirectory, LR"(Task Settings)"), true);

		delete xLSearchTask2;
		delete xLSearchTask;
	}

	void XLTest::XLSearchWithGeneratedIndices()
	{
		XLSearchTask *xlSearchTask = new XLSearchTask();
		CommonParameters tempVar();
		xlSearchTask->setCommonParameters(&tempVar);
		std::wstring myFile = FileSystem::combine(TestContext::CurrentContext->TestDirectory, LR"(XlTestData\BSA_DSSO_ETchD6010.mgf)");
		std::wstring myDatabase = FileSystem::combine(TestContext::CurrentContext->TestDirectory, LR"(XlTestData\BSA.fasta)");
		std::wstring folderPath = FileSystem::combine(TestContext::CurrentContext->TestDirectory, LR"(TestXLSearch)");
		DbForTask *db = new DbForTask(myDatabase, false);
		std::vector<(std::wstring, MetaMorpheusTask)*> taskList = {(L"TestXLSearch", xlSearchTask)};
		FileSystem::createDirectory(folderPath);

		//creates .params files if they do not exist
		xlSearchTask->RunTask(folderPath, {db}, {myFile}, L"normal");
		//tests .params files
		xlSearchTask->RunTask(folderPath, {db}, {myFile}, L"normal");

		auto baseDir = FileSystem::getDirectoryName(db->getFilePath());
		auto directory = new DirectoryInfo(baseDir);
		std::vector<DirectoryInfo*> directories = directory->GetDirectories();
		for (auto possibleFolder : directories)
		{
			if (FileSystem::fileExists(FileSystem::combine(possibleFolder->FullName, L"indexEngine.params")))
			{
				File::Delete(possibleFolder->GetFiles().ElementAt(0)->FullName);
			}
		}
		//tests without .params files
		xlSearchTask->RunTask(folderPath, {db}, {myFile}, L"normal");

		auto lines = File::ReadAllLines(FileSystem::combine(folderPath, LR"(XL_Intralinks.tsv)"));
		Assert::That(lines.size() == 2);
		Directory::Delete(folderPath, true);
		Directory::Delete(FileSystem::combine(TestContext::CurrentContext->TestDirectory, LR"(Task Settings)"), true);

		delete directory;
//C# TO C++ CONVERTER TODO TASK: A 'delete db' statement was not added since db was passed to a method or constructor. Handle memory management manually.
//C# TO C++ CONVERTER TODO TASK: A 'delete xlSearchTask' statement was not added since xlSearchTask was passed to a method or constructor. Handle memory management manually.
	}

	void XLTest::TestGetPossibleCrosslinkerSites()
	{
		PeptideWithSetModifications *peptide = new PeptideWithSetModifications(L"PEPTIDE", nullptr);
		std::vector<int> sites = CrosslinkSpectralMatch::GetPossibleCrosslinkerModSites({L'P'}, peptide);
		Assert::That(sites.SequenceEqual(std::vector<int> {1, 3}));

//C# TO C++ CONVERTER TODO TASK: A 'delete peptide' statement was not added since peptide was passed to a method or constructor. Handle memory management manually.
	}

	void XLTest::TestTheoreticalFragmentsLoop()
	{
		Protein *p = new Protein(L"PEPTIDE", L"");
		DigestionParams tempVar();
		auto peptide = p->Digest(&tempVar, std::vector<Modification*>(), std::vector<Modification*>()).First();

		ModificationMotif motif;
		ModificationMotif::TryGetMotif(L"X", motif);
		auto loopMod = new Modification(L"Loop", _modificationType: L"XLTest", _target: motif, _locationRestriction: L"Anywhere.", _monoisotopicMass: 10000);

		auto loopLocationsWithFragments = CrosslinkedPeptide::XlLoopGetTheoreticalFragments(DissociationType::HCD, loopMod, std::vector<int> {3, 5}, peptide);

		Assert::That(loopLocationsWithFragments.size() == 1);
		auto loopLocationWithFragments = loopLocationsWithFragments.First();

		Assert::That(loopLocationWithFragments->Key->Item1 == 3);
		Assert::That(loopLocationWithFragments->Key->Item2 == 5);

		auto fragments = loopLocationWithFragments->Value;

		auto bIons = fragments->Where([&] (std::any v)
		{
//C# TO C++ CONVERTER TODO TASK: A 'delete loopMod' statement was not added since loopMod was passed to a method or constructor. Handle memory management manually.
		delete p;
			return v->ProductType == ProductType::b;
		}).ToList();
		Assert::That(bIons.Select([&] (std::any v)
		{
			(int)v.NeutralMass;
		}).SequenceEqual(std::vector<int> {97, 226, 10537, 10652}));

		auto yIons = fragments->Where([&] (std::any v)
		{
//C# TO C++ CONVERTER TODO TASK: A 'delete loopMod' statement was not added since loopMod was passed to a method or constructor. Handle memory management manually.
		delete p;
			return v->ProductType == ProductType::y;
		}).ToList();
		Assert::That(yIons.Select([&] (std::any v)
		{
			(int)v.NeutralMass;
		}).SequenceEqual(std::vector<int> {147, 262, 10573, 10702}));

//C# TO C++ CONVERTER TODO TASK: A 'delete loopMod' statement was not added since loopMod was passed to a method or constructor. Handle memory management manually.
		delete p;
	}

	void XLTest::TestTheoreticalLoopFragmentsWithMod()
	{
		std::any tMotif;
		ModificationMotif::TryGetMotif(L"T", tMotif);
		Modification *phospho = new Modification(_originalId: L"Phospho", _modificationType: L"Mod", _locationRestriction: L"Anywhere.", _monoisotopicMass: 79.98, _target: tMotif);
		std::vector<int> modPositions = {2, 3, 4, 5, 6};

		for (auto modPosition : modPositions)
		{
			std::unordered_map<int, std::vector<Modification*>> oneBasedMods =
			{
				{
					modPosition, {phospho}
				}
			};
			Protein *p = new Protein(L"PTTTTTE", L"", oneBasedModifications: oneBasedMods);
			DigestionParams tempVar();
			auto peptide = p->Digest(&tempVar, std::vector<Modification*>(), std::vector<Modification*>()).Where([&] (std::any v)
			{
			delete p;
			delete phospho;
				return v::AllModsOneIsNterminus->Count == 1;
			}).First();

			ModificationMotif motif;
			ModificationMotif::TryGetMotif(L"X", motif);
			auto loopMod = new Modification(L"Loop", _modificationType: L"XLTest", _target: motif, _locationRestriction: L"Anywhere.", _monoisotopicMass: 10000);

			auto loopLocationsWithFragments = CrosslinkedPeptide::XlLoopGetTheoreticalFragments(DissociationType::HCD, loopMod, std::vector<int> {3, 5}, peptide);

			Assert::That(loopLocationsWithFragments.size() == 1);
			auto loopLocationWithFragments = loopLocationsWithFragments.First();

			Assert::That(loopLocationWithFragments->Key->Item1 == 3);
			Assert::That(loopLocationWithFragments->Key->Item2 == 5);

			auto fragments = loopLocationWithFragments->Value;

			auto bIons = fragments->Where([&] (std::any v)
			{
//C# TO C++ CONVERTER TODO TASK: A 'delete loopMod' statement was not added since loopMod was passed to a method or constructor. Handle memory management manually.
			delete p;
			delete phospho;
				return v->ProductType == ProductType::b;
			}).ToList();
			auto yIons = fragments->Where([&] (std::any v)
			{
//C# TO C++ CONVERTER TODO TASK: A 'delete loopMod' statement was not added since loopMod was passed to a method or constructor. Handle memory management manually.
			delete p;
			delete phospho;
				return v->ProductType == ProductType::y;
			}).ToList();

			if (modPosition == 2)
			{
				//             _
				//            | |
				// PT[Phospho]TTTTE
				Assert::That(bIons.Select([&] (std::any v)
				{
					(int)v.NeutralMass;
				}).SequenceEqual(std::vector<int> {97, 278, 10581, 10682}));
				Assert::That(yIons.Select([&] (std::any v)
				{
					(int)v.NeutralMass;
				}).SequenceEqual(std::vector<int> {147, 248, 10551, 10732}));
			}
			else if (modPosition == 3)
			{
				//    __________
				//   |          |
				// PTT[Phospho]TTTE
				Assert::That(bIons.Select([&] (std::any v)
				{
					(int)v.NeutralMass;
				}).SequenceEqual(std::vector<int> {97, 198, 10581, 10682}));
				Assert::That(yIons.Select([&] (std::any v)
				{
					(int)v.NeutralMass;
				}).SequenceEqual(std::vector<int> {147, 248, 10631, 10732}));
			}
			else if (modPosition == 4)
			{
				//    __________
				//   |          |
				// PTTT[Phospho]TTE
				Assert::That(bIons.Select([&] (std::any v)
				{
					(int)v.NeutralMass;
				}).SequenceEqual(std::vector<int> {97, 198, 10581, 10682}));
				Assert::That(yIons.Select([&] (std::any v)
				{
					(int)v.NeutralMass;
				}).SequenceEqual(std::vector<int> {147, 248, 10631, 10732}));
			}
			else if (modPosition == 5)
			{
				//    _
				//   | |
				// PTTTT[Phospho]TE
				Assert::That(bIons.Select([&] (std::any v)
				{
					(int)v.NeutralMass;
				}).SequenceEqual(std::vector<int> {97, 198, 10581, 10682}));
				Assert::That(yIons.Select([&] (std::any v)
				{
					(int)v.NeutralMass;
				}).SequenceEqual(std::vector<int> {147, 248, 10631, 10732}));
			}
			else if (modPosition == 6)
			{
				//    _
				//   | |
				// PTTTTT[Phospho]E
				Assert::That(bIons.Select([&] (std::any v)
				{
					(int)v.NeutralMass;
				}).SequenceEqual(std::vector<int> {97, 198, 10501, 10682}));
				Assert::That(yIons.Select([&] (std::any v)
				{
					(int)v.NeutralMass;
				}).SequenceEqual(std::vector<int> {147, 328, 10631, 10732}));
			}

//C# TO C++ CONVERTER TODO TASK: A 'delete loopMod' statement was not added since loopMod was passed to a method or constructor. Handle memory management manually.
			delete p;
		}

		delete phospho;
	}

	void XLTest::TestDeadendTris()
	{
		Protein *protein = new Protein(L"PEPTIDE", L"");
		auto csms = std::vector<CrosslinkSpectralMatch*>(1);

		// generate the scan with the deadend mod peptide's fragments
		auto scans = std::vector<Ms2ScanWithSpecificMass*>(1);
		std::any motif;
		ModificationMotif::TryGetMotif(L"T", motif);
		auto crosslinker = new Crosslinker(L"T", L"T", L"test", false, 100, 0, 0, 0, 0, 0, 50);
		Modification *deadend = new Modification(L"TestId", _target: motif, _locationRestriction: L"Anywhere.", _monoisotopicMass: crosslinker->getDeadendMassTris(), _modificationType: L"Test");

		DigestionParams tempVar();
		auto deadendPeptide = protein->Digest(&tempVar, std::vector<Modification*> {deadend}, std::vector<Modification*>()).First();

		std::vector<double> mz = deadendPeptide->Fragment(DissociationType::HCD, FragmentationTerminus::Both)->Select([&] (std::any p)
		{
			p::NeutralMass::ToMz(1);
		}).OrderBy([&] (std::any v)
		{
//C# TO C++ CONVERTER TODO TASK: A 'delete deadend' statement was not added since deadend was passed to a method or constructor. Handle memory management manually.
		delete crosslinker;
		delete protein;
			return v;
		})->ToArray();
		std::vector<double> intensities = {1.0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

		MzSpectrum *spectrum = new MzSpectrum(mz, intensities, false);
		MsDataScan *sc = new MsDataScan(spectrum, 1, 2, true, Polarity::Positive, 1, spectrum->Range, L"", MZAnalyzerType::Orbitrap, 12, 1.0, nullptr, nullptr);
		CommonParameters tempVar2();
		scans[0] = new Ms2ScanWithSpecificMass(sc, deadendPeptide->MonoisotopicMass.ToMz(2), 2, L"", &tempVar2);

		// search the data with the peptide WITHOUT the deadend mod annotated in the search database.
		// the search engine should be able to correctly identify the deadend mod on T
		IndexingEngine tempVar3({protein}, new std::vector<Modification*>(), new std::vector<Modification*>(), 0, DecoyType::None, new CommonParameters(), 1000, false, new std::vector<FileInfo*>(), new std::vector<std::wstring>());
		auto indexingResults = static_cast<IndexingResults*>((&tempVar3)->Run());

		CrosslinkSearchEngine tempVar4(csms, scans, indexingResults->getPeptideIndex(), indexingResults->getFragmentIndex(), 0, new CommonParameters(), crosslinker, false, 0, false, false, true, new std::vector<std::wstring>());
		(&tempVar4)->Run();

		CrosslinkSpectralMatch *csm = csms.First();
		Assert::That(csm->getCrossType() == PsmCrossType::DeadEndTris);
		Assert::That(csm->MatchedFragmentIons->Count == 12);

//C# TO C++ CONVERTER TODO TASK: A 'delete sc' statement was not added since sc was passed to a method or constructor. Handle memory management manually.
//C# TO C++ CONVERTER TODO TASK: A 'delete spectrum' statement was not added since spectrum was passed to a method or constructor. Handle memory management manually.
//C# TO C++ CONVERTER TODO TASK: A 'delete deadend' statement was not added since deadend was passed to a method or constructor. Handle memory management manually.
//C# TO C++ CONVERTER TODO TASK: A 'delete crosslinker' statement was not added since crosslinker was passed to a method or constructor. Handle memory management manually.
//C# TO C++ CONVERTER TODO TASK: A 'delete protein' statement was not added since protein was passed to a method or constructor. Handle memory management manually.
	}

	void XLTest::TestTheoreticalFragmentsNonCleavableCrosslink()
	{
		Crosslinker *c = new Crosslinker(L"P", L"R", L"Test", false, 1000, 0, 0, 1000, 5, 5, 5);

		Protein *p1 = new Protein(L"PEPTIDE", L"");
		DigestionParams tempVar();
		auto alphaPeptide = p1->Digest(&tempVar, std::vector<Modification*>(), std::vector<Modification*>()).First();

		Protein *p2 = new Protein(L"PRLTEIN", L"");
		DigestionParams tempVar2();
		auto betaPeptide = p2->Digest(&tempVar2, std::vector<Modification*>(), std::vector<Modification*>()).Where([&] (std::any v)
		{
		delete p2;
		delete p1;
		delete c;
			return v->MissedCleavages == 1;
		}).First();

		auto theoreticalCrosslinkFragments = CrosslinkedPeptide::XlGetTheoreticalFragments(DissociationType::HCD, c, std::vector<int> {3}, betaPeptide->MonoisotopicMass, alphaPeptide).ToList();

		Assert::That(theoreticalCrosslinkFragments.size() == 1);
		auto loopLocationWithFragments = theoreticalCrosslinkFragments.front();

		Assert::That(loopLocationWithFragments->Item1 == 3);

		auto fragments = loopLocationWithFragments->Item2;

		auto bIons = fragments->Where([&] (std::any v)
		{
		delete p2;
		delete p1;
//C# TO C++ CONVERTER TODO TASK: A 'delete c' statement was not added since c was passed to a method or constructor. Handle memory management manually.
			return v->ProductType == ProductType::b;
		}).ToList();
		Assert::That(bIons.Select([&] (std::any v)
		{
			(int)v.NeutralMass;
		}).SequenceEqual(std::vector<int> {97, 226, 2164, 2265, 2378, 2493}));

		auto yIons = fragments->Where([&] (std::any v)
		{
		delete p2;
		delete p1;
//C# TO C++ CONVERTER TODO TASK: A 'delete c' statement was not added since c was passed to a method or constructor. Handle memory management manually.
			return v->ProductType == ProductType::y;
		}).ToList();
		Assert::That(yIons.Select([&] (std::any v)
		{
			(int)v.NeutralMass;
		}).SequenceEqual(std::vector<int> {147, 262, 375, 476, 2414, 2543}));

		delete p2;
		delete p1;
//C# TO C++ CONVERTER TODO TASK: A 'delete c' statement was not added since c was passed to a method or constructor. Handle memory management manually.
	}

	void XLTest::TestTheoreticalFragmentsCleavableCrosslink()
	{
		Crosslinker *c = new Crosslinker(L"P", L"R", L"Test", true, 1000, 15, 25, 1000, 5, 5, 5);

		Protein *p1 = new Protein(L"PEPTIDE", L"");
		DigestionParams tempVar();
		auto alphaPeptide = p1->Digest(&tempVar, std::vector<Modification*>(), std::vector<Modification*>()).First();

		Protein *p2 = new Protein(L"PRLTEIN", L"");
		DigestionParams tempVar2();
		auto betaPeptide = p2->Digest(&tempVar2, std::vector<Modification*>(), std::vector<Modification*>()).Where([&] (std::any v)
		{
		delete p2;
		delete p1;
		delete c;
			return v->MissedCleavages == 1;
		}).First();

		auto theoreticalCrosslinkFragments = CrosslinkedPeptide::XlGetTheoreticalFragments(DissociationType::HCD, c, std::vector<int> {3}, 10000, alphaPeptide).ToList();

		Assert::That(theoreticalCrosslinkFragments.size() == 1);

		// cleaved fragments
		auto linkLocationWithFragments = theoreticalCrosslinkFragments[0];

		Assert::That(linkLocationWithFragments.Item1 == 3);
		auto fragmentsWithCleavedXlPieces = linkLocationWithFragments.Item2;

		auto bIons = fragmentsWithCleavedXlPieces->Where([&] (std::any v)
		{
		delete p2;
		delete p1;
//C# TO C++ CONVERTER TODO TASK: A 'delete c' statement was not added since c was passed to a method or constructor. Handle memory management manually.
			return v->ProductType == ProductType::b;
		}).ToList();
		Assert::That(bIons.Select([&] (std::any v)
		{
			(int)v.NeutralMass;
		}).SequenceEqual(std::vector<int> {97, 226, 338, 439, 552, 667, 348, 449, 562, 677}));

		auto yIons = fragmentsWithCleavedXlPieces->Where([&] (std::any v)
		{
		delete p2;
		delete p1;
//C# TO C++ CONVERTER TODO TASK: A 'delete c' statement was not added since c was passed to a method or constructor. Handle memory management manually.
			return v->ProductType == ProductType::y;
		}).ToList();
		Assert::That(yIons.Select([&] (std::any v)
		{
			(int)v.NeutralMass;
		}).SequenceEqual(std::vector<int> {147, 262, 375, 476, 588, 717, 598, 727}));

		auto signatureIons = fragmentsWithCleavedXlPieces->Where([&] (std::any v)
		{
		delete p2;
		delete p1;
//C# TO C++ CONVERTER TODO TASK: A 'delete c' statement was not added since c was passed to a method or constructor. Handle memory management manually.
			return v->ProductType == ProductType::M;
		}).ToList();
		Assert::That(signatureIons.size() == 2);
		Assert::That(signatureIons.Select([&] (std::any v)
		{
			(int)v.NeutralMass;
		}).SequenceEqual(std::vector<int> {814, 824}));

		delete p2;
		delete p1;
//C# TO C++ CONVERTER TODO TASK: A 'delete c' statement was not added since c was passed to a method or constructor. Handle memory management manually.
	}

	void XLTest::TestWriteToPercolator()
	{
		XLSearchTask *xlst = new XLSearchTask();
		XlSearchParameters tempVar();
		xlst->setXlSearchParameters(&tempVar);
		xlst->getXlSearchParameters()->setWriteOutputForPercolator(true);

		std::wstring myFileXl = FileSystem::combine(TestContext::CurrentContext->TestDirectory, LR"(XlTestData\BSA_DSSO_ETchD6010.mgf)");
		std::wstring myDatabaseXl = FileSystem::combine(TestContext::CurrentContext->TestDirectory, LR"(XlTestData\BSA.fasta)");
		std::wstring outputFolder = FileSystem::combine(TestContext::CurrentContext->TestDirectory, LR"(TestXLSearch)");
		DbForTask *db = new DbForTask(myDatabaseXl, false);

		std::vector<(std::wstring, MetaMorpheusTask)*> taskList = {(L"TestPercolator", xlst)};

		auto engine = new EverythingRunnerEngine(taskList, std::vector<std::wstring> {myFileXl}, std::vector<DbForTask*> {db}, outputFolder);
		engine->Run();

		auto results = FileSystem::combine(outputFolder, LR"(TestPercolator\XL_Intralinks_Percolator.txt)");
		auto lines = File::ReadAllLines(results);
		Assert::That(lines[0] == L"SpecId\tLabel\tScannr\tScore\tdScore\tNormRank\tCharge\tMass\tPPM\tLenShort\tLenLong\tLenSum\tPeptide\tProtein");
		Assert::That(lines[1] == L"T-1-30.6190992666667\t1\t1\t20.0641008915522\t0\t7\t3\t1994.05202313843\t0.664979354397676\t7\t9\t16\t-.EKVLTSSAR2--LSQKFPK4.-\t3336842(211)\t3336842(245)");
		Directory::Delete(outputFolder, true);

		delete engine;
//C# TO C++ CONVERTER TODO TASK: A 'delete db' statement was not added since db was passed to a method or constructor. Handle memory management manually.
//C# TO C++ CONVERTER TODO TASK: A 'delete xlst' statement was not added since xlst was passed to a method or constructor. Handle memory management manually.
	}

	void XLTest::TestWriteNonSingleCross()
	{
		XLSearchTask *xlst = new XLSearchTask();
		Protein *protein = new Protein(L"PEPTIDE", L"");
		auto csms = std::vector<CrosslinkSpectralMatch*>(1);

		// generate the scan with the deadend mod peptide's fragments
		auto scans = std::vector<Ms2ScanWithSpecificMass*>(1);
		std::any motif;
		ModificationMotif::TryGetMotif(L"T", motif);
		auto crosslinker = new Crosslinker(L"T", L"T", L"test", false, 100, 0, 0, 0, 0, 0, 50);
		Modification *deadend = new Modification(L"TestId", _target: motif, _locationRestriction: L"Anywhere.", _monoisotopicMass: crosslinker->getDeadendMassTris(), _modificationType: L"Test");

		DigestionParams tempVar();
		auto deadendPeptide = protein->Digest(&tempVar, std::vector<Modification*> {deadend}, std::vector<Modification*>()).First();

		std::vector<double> mz = deadendPeptide->Fragment(DissociationType::HCD, FragmentationTerminus::Both)->Select([&] (std::any p)
		{
			p::NeutralMass::ToMz(1);
		}).OrderBy([&] (std::any v)
		{
//C# TO C++ CONVERTER TODO TASK: A 'delete deadend' statement was not added since deadend was passed to a method or constructor. Handle memory management manually.
		delete crosslinker;
		delete protein;
		delete xlst;
			return v;
		})->ToArray();
		std::vector<double> intensities = {1.0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

		MzSpectrum *spectrum = new MzSpectrum(mz, intensities, false);
		MsDataScan *sc = new MsDataScan(spectrum, 1, 2, true, Polarity::Positive, 1, spectrum->Range, L"", MZAnalyzerType::Orbitrap, 12, 1.0, nullptr, nullptr);
		CommonParameters tempVar2();
		scans[0] = new Ms2ScanWithSpecificMass(sc, deadendPeptide->MonoisotopicMass.ToMz(2), 2, L"", &tempVar2);

		IndexingEngine tempVar3({protein}, new std::vector<Modification*>(), new std::vector<Modification*>(), 0, DecoyType::None, new CommonParameters(), 1000, false, new std::vector<FileInfo*>(), new std::vector<std::wstring>());
		auto indexingResults = static_cast<IndexingResults*>((&tempVar3)->Run());

		CrosslinkSearchEngine tempVar4(csms, scans, indexingResults->getPeptideIndex(), indexingResults->getFragmentIndex(), 0, new CommonParameters(), crosslinker, false, 0, false, false, true, new std::vector<std::wstring>());
		(&tempVar4)->Run();

		csms[0]->SetFdrValues(0, 0, 0.1, 0, 0, 0, 0, 0, 0, false);

		xlst->WritePepXML_xl(csms.ToList(), std::vector<Protein*>(), L"", {deadend}, {deadend}, std::vector<std::wstring>(), TestContext::CurrentContext->TestDirectory, L"test", std::vector<std::wstring>());
		File::Delete(FileSystem::combine(TestContext::CurrentContext->TestDirectory, LR"(test.pep.XML)"));

//C# TO C++ CONVERTER TODO TASK: A 'delete sc' statement was not added since sc was passed to a method or constructor. Handle memory management manually.
//C# TO C++ CONVERTER TODO TASK: A 'delete spectrum' statement was not added since spectrum was passed to a method or constructor. Handle memory management manually.
//C# TO C++ CONVERTER TODO TASK: A 'delete deadend' statement was not added since deadend was passed to a method or constructor. Handle memory management manually.
//C# TO C++ CONVERTER TODO TASK: A 'delete crosslinker' statement was not added since crosslinker was passed to a method or constructor. Handle memory management manually.
//C# TO C++ CONVERTER TODO TASK: A 'delete protein' statement was not added since protein was passed to a method or constructor. Handle memory management manually.
		delete xlst;
	}

	XLTestDataFile::XLTestDataFile() : MsDataFile(nullptr, nullptr, nullptr, nullptr, nullptr)
	{
		auto mz1 = std::vector<double> {1994.05.ToMz(3), 846.4963.ToMz(1), 1004.495.ToMz(1), 1093.544.ToMz(1), 1043.561.ToMz(1)};
		auto intensities1 = std::vector<double> {1, 1, 1, 1, 1};
		auto MassSpectrum1 = new MzSpectrum(mz1, intensities1, false);
		auto ScansHere = std::vector<MsDataScan*>();
		MzLibUtil::MzRange tempVar2(0, 10000);
		ScansHere.new MsDataScan(MassSpectrum1, 1, 1, true, Polarity::Positive, 1, &tempVar2, L"ff", MZAnalyzerType::Unknown, 1000, 1, nullptr, L"scan=1");

		auto mz2 = std::vector<double> {100, 201.1234, 244.1656, 391.2340, 420.2201, 521.2678, 634.3519, 889.965, 1044.568, 1094.551, 1279.671, 1378.74, 1491.824};
		auto intensities2 = std::vector<double> {100, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
		auto MassSpectrum2 = new MzSpectrum(mz2, intensities2, false);
		MsDataScan tempVar3(MassSpectrum2, 2, 2, true, Polarity::Positive, 1.0, new MzLibUtil::MzRange(0, 10000), L"f", MZAnalyzerType::Unknown, 112, 1.0, nullptr, L"scan=2", 1994.05.ToMz(3), 3, 1, 1994.05.ToMz(3), 2, DissociationType::HCD, 1, 1994.05.ToMz(3));
		ScansHere.push_back(&tempVar3);

		auto mz3 = std::vector<double> {100, 201.1234, 244.1656, 391.2340};
		auto intensities3 = std::vector<double> {100, 1, 1, 1};
		auto MassSpectrum3 = new MzSpectrum(mz3, intensities3, false);
		MsDataScan tempVar4(MassSpectrum3, 3, 2, true, Polarity::Positive, 1.0, new MzLibUtil::MzRange(0, 10000), L"f", MZAnalyzerType::Unknown, 103, 1.0, nullptr, L"scan=3", 846.4963.ToMz(1), 1, 1, 846.4963.ToMz(1), 2, DissociationType::HCD, 1, 846.4963.ToMz(1));
		ScansHere.push_back(&tempVar4);

		auto mz4 = std::vector<double> {100, 201.1234, 244.1656, 391.2340};
		auto intensities4 = std::vector<double> {100, 1, 1, 1};
		auto MassSpectrum4 = new MzSpectrum(mz4, intensities4, false);
		MsDataScan tempVar5(MassSpectrum4, 4, 2, true, Polarity::Positive, 1.0, new MzLibUtil::MzRange(0, 10000), L"f", MZAnalyzerType::Unknown, 103, 1.0, nullptr, L"scan=4", 1004.491.ToMz(1), 1, 1, 1004.491.ToMz(1), 2, DissociationType::HCD, 1, 1004.491.ToMz(1));
		ScansHere.push_back(&tempVar5);

		Scans = ScansHere.ToArray();

//C# TO C++ CONVERTER TODO TASK: A 'delete MassSpectrum4' statement was not added since MassSpectrum4 was passed to a method or constructor. Handle memory management manually.
//C# TO C++ CONVERTER TODO TASK: A 'delete MassSpectrum3' statement was not added since MassSpectrum3 was passed to a method or constructor. Handle memory management manually.
//C# TO C++ CONVERTER TODO TASK: A 'delete MassSpectrum2' statement was not added since MassSpectrum2 was passed to a method or constructor. Handle memory management manually.
//C# TO C++ CONVERTER TODO TASK: A 'delete MassSpectrum1' statement was not added since MassSpectrum1 was passed to a method or constructor. Handle memory management manually.
	}

	std::wstring XLTestDataFile::getFilePath() const
	{
		return L"XLTestDataFile";
	}

	std::wstring XLTestDataFile::getName() const
	{
		return L"XLTestDataFile";
	}

	void XLTestDataFile::ReplaceFirstScanArrays(std::vector<double> &mz, std::vector<double> &intensities)
	{
		MzSpectrum *massSpectrum = new MzSpectrum(mz, intensities, false);
		Scans[0] = new MsDataScan(massSpectrum, Scans[0].OneBasedScanNumber, Scans[0].MsnOrder, Scans[0].IsCentroid, Scans[0].Polarity, Scans[0].RetentionTime, Scans[0].ScanWindowRange, Scans[0].ScanFilter, Scans[0].MzAnalyzer, massSpectrum->SumOfAllY, Scans[0].InjectionTime, nullptr, Scans[0].NativeId);

//C# TO C++ CONVERTER TODO TASK: A 'delete massSpectrum' statement was not added since massSpectrum was passed to a method or constructor. Handle memory management manually.
	}

	XLTestDataFileDiffSite::XLTestDataFileDiffSite() : MsDataFile(nullptr, nullptr, nullptr, nullptr, nullptr)
	{
		auto mz1 = std::vector<double> {100, 1030.5956.ToMz(1)};
		auto intensities1 = std::vector<double> {100, 1};
		auto MassSpectrum1 = new MzSpectrum(mz1, intensities1, false);
		auto ScansHere = std::vector<MsDataScan*>();
		MzLibUtil::MzRange tempVar2(0, 10000);
		ScansHere.new MsDataScan(MassSpectrum1, 1, 1, true, Polarity::Positive, 1, &tempVar2, L"ff", MZAnalyzerType::Unknown, 1000, 1, nullptr, L"scan=1");

		auto mz2 = std::vector<double> {100, 147.1128, 175.119, 213.1598, 246.1561, 275.1714, 757.4388, 786.4541, 819.4504, 857.4912, 885.4974, 918.5189, 932.5345};
		auto intensities2 = std::vector<double> {100, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
		auto MassSpectrum2 = new MzSpectrum(mz2, intensities2, false);
		MsDataScan tempVar3(MassSpectrum2, 2, 2, true, Polarity::Positive, 1.0, new MzLibUtil::MzRange(0, 10000), L"f", MZAnalyzerType::Unknown, 112, 1.0, nullptr, L"scan=2", 1030.5956.ToMz(1), 1, 1, 1030.5956.ToMz(1), 2, DissociationType::HCD, 1, 1030.5956.ToMz(1));
		ScansHere.push_back(&tempVar3);

		Scans = ScansHere.ToArray();

//C# TO C++ CONVERTER TODO TASK: A 'delete MassSpectrum2' statement was not added since MassSpectrum2 was passed to a method or constructor. Handle memory management manually.
//C# TO C++ CONVERTER TODO TASK: A 'delete MassSpectrum1' statement was not added since MassSpectrum1 was passed to a method or constructor. Handle memory management manually.
	}

	std::wstring XLTestDataFileDiffSite::getFilePath() const
	{
		return L"XLTestDataFileDiffSite";
	}

	std::wstring XLTestDataFileDiffSite::getName() const
	{
		return L"XLTestDataFileDiffSite";
	}

	void XLTestDataFileDiffSite::ReplaceFirstScanArrays(std::vector<double> &mz, std::vector<double> &intensities)
	{
		MzSpectrum *massSpectrum = new MzSpectrum(mz, intensities, false);
		Scans[0] = new MsDataScan(massSpectrum, Scans[0].OneBasedScanNumber, Scans[0].MsnOrder, Scans[0].IsCentroid, Scans[0].Polarity, Scans[0].RetentionTime, Scans[0].ScanWindowRange, Scans[0].ScanFilter, Scans[0].MzAnalyzer, massSpectrum->SumOfAllY, Scans[0].InjectionTime, nullptr, Scans[0].NativeId);

//C# TO C++ CONVERTER TODO TASK: A 'delete massSpectrum' statement was not added since massSpectrum was passed to a method or constructor. Handle memory management manually.
	}
}
