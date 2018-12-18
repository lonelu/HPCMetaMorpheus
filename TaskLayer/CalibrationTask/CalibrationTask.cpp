﻿#include "CalibrationTask.h"
#include "CalibrationParameters.h"
#include "../../EngineLayer/CommonParameters.h"
#include "../DbForTask.h"
#include "../FileSpecificParameters.h"
#include "../MyTaskResults.h"
#include "../MyFileManager.h"
#include "../../EngineLayer/GlobalVariables.h"
#include "../../EngineLayer/EventArgs/ProgressEventArgs.h"
#include "../../EngineLayer/PrecursorSearchModes/MassDiffAcceptor.h"
#include "../../EngineLayer/PrecursorSearchModes/SingleAbsoluteAroundZeroMassDiffAcceptor.h"
#include "../../EngineLayer/PrecursorSearchModes/SinglePpmAroundZeroMassDiffAcceptor.h"
#include "../../EngineLayer/PeptideSpectralMatch.h"

using namespace EngineLayer;
using namespace EngineLayer::Calibration;
using namespace EngineLayer::ClassicSearch;
using namespace EngineLayer::FdrAnalysis;
using namespace IO::MzML;
using namespace MassSpectrometry;
using namespace MzLibUtil;
using namespace Nett;
using namespace Proteomics;
using namespace Proteomics::Fragmentation;
using namespace UsefulProteomicsDatabases;

namespace TaskLayer
{

	CalibrationTask::CalibrationTask() : MetaMorpheusTask(MyTask::Calibrate)
	{
		EngineLayer::CommonParameters tempVar(, , false, , , , , , , , , , , false, , , new PpmTolerance(25), new PpmTolerance(15));
		setCommonParameters(&tempVar);

		TaskLayer::CalibrationParameters tempVar2();
		setCalibrationParameters(&tempVar2);
	}

	TaskLayer::CalibrationParameters *CalibrationTask::getCalibrationParameters() const
	{
		return privateCalibrationParameters;
	}

	void CalibrationTask::setCalibrationParameters(TaskLayer::CalibrationParameters *value)
	{
		privateCalibrationParameters = value;
	}

	MyTaskResults *CalibrationTask::RunSpecific(const std::wstring &OutputFolder, std::vector<DbForTask*> &dbFilenameList, std::vector<std::wstring> &currentRawFileList, const std::wstring &taskId, std::vector<FileSpecificParameters*> &fileSettingsList)
	{
		std::vector<Modification> variableModifications;
		std::vector<Modification> fixedModifications;
		std::vector<string> localizeableModificationTypes;
		LoadModifications(taskId, variableModifications, fixedModifications, localizeableModificationTypes);

		// load proteins
		std::vector<Protein*> proteinList = LoadProteins(taskId, dbFilenameList, true, DecoyType::Reverse, localizeableModificationTypes, getCommonParameters());

		// write prose settings
		ProseCreatedWhileRunning->append(L"The following calibration settings were used: ");
		ProseCreatedWhileRunning->append(L"protease = " + getCommonParameters()->getDigestionParams()->Protease + L"; ");
		ProseCreatedWhileRunning->append(L"maximum missed cleavages = " + getCommonParameters()->getDigestionParams()->MaxMissedCleavages + L"; ");
		ProseCreatedWhileRunning->append(L"minimum peptide length = " + getCommonParameters()->getDigestionParams()->MinPeptideLength + L"; ");
		ProseCreatedWhileRunning->append(getCommonParameters()->getDigestionParams()->MaxPeptideLength == std::numeric_limits<int>::max() ? L"maximum peptide length = unspecified; " : L"maximum peptide length = " + getCommonParameters()->getDigestionParams()->MaxPeptideLength + L"; ");
		ProseCreatedWhileRunning->append(L"initiator methionine behavior = " + getCommonParameters()->getDigestionParams()->InitiatorMethionineBehavior + L"; ");
		ProseCreatedWhileRunning->append(L"fixed modifications = " + std::wstring::Join(L", ", fixedModifications->Select([&] (std::any m)
		{
			m::IdWithMotif;
		})) + L"; ");
		ProseCreatedWhileRunning->append(L"variable modifications = " + std::wstring::Join(L", ", variableModifications->Select([&] (std::any m)
		{
			m::IdWithMotif;
		})) + L"; ");
		ProseCreatedWhileRunning->append(L"max mods per peptide = " + getCommonParameters()->getDigestionParams()->MaxModsForPeptide + L"; ");
		ProseCreatedWhileRunning->append(L"max modification isoforms = " + getCommonParameters()->getDigestionParams()->MaxModificationIsoforms + L"; ");
		ProseCreatedWhileRunning->append(L"precursor mass tolerance = " + getCommonParameters()->getPrecursorMassTolerance() + L"; ");
		ProseCreatedWhileRunning->append(L"product mass tolerance = " + getCommonParameters()->getProductMassTolerance() + L". ");
		ProseCreatedWhileRunning->append(L"The combined search database contained " + proteinList.size()([&] (std::any p)
		{
			!p::IsDecoy;
		}) + L" non-decoy protein entries including " + proteinList.size()([&] (std::any p)
		{
			p::IsContaminant;
		}) + L" contaminant sequences. ");

		// start the calibration task
		Status(L"Calibrating...", std::vector<std::wstring> {taskId});
		MyTaskResults = new MyTaskResults(this);
		MyTaskResults->NewSpectra = std::vector<std::wstring>();
		MyTaskResults->NewFileSpecificTomls = std::vector<std::wstring>();

		auto myFileManager = new MyFileManager(true);
		std::vector<std::wstring> spectraFilesAfterCalibration;

		for (int spectraFileIndex = 0; spectraFileIndex < currentRawFileList.size(); spectraFileIndex++)
		{
			if (GlobalVariables::getStopLoops())
			{
				break;
			}

			bool couldNotFindEnoughDatapoints = false;

			// get filename stuff
			auto originalUncalibratedFilePath = currentRawFileList[spectraFileIndex];
			auto originalUncalibratedFilenameWithoutExtension = Path::GetFileNameWithoutExtension(originalUncalibratedFilePath);
			std::wstring calibratedFilePath = FileSystem::combine(OutputFolder, originalUncalibratedFilenameWithoutExtension + CalibSuffix + L".mzML");

			// mark the file as in-progress
			StartingDataFile(originalUncalibratedFilePath, std::vector<std::wstring> {taskId, L"Individual Spectra Files", originalUncalibratedFilePath});

			EngineLayer::CommonParameters *combinedParams = SetAllFileSpecificCommonParams(getCommonParameters(), fileSettingsList[spectraFileIndex]);

			// load the file
			Status(L"Loading spectra file...", std::vector<std::wstring> {taskId, L"Individual Spectra Files"});

			auto myMsDataFile = myFileManager->LoadFile(originalUncalibratedFilePath, std::make_optional(getCommonParameters()->getTopNpeaks()), std::make_optional(getCommonParameters()->getMinRatio()), getCommonParameters()->getTrimMs1Peaks(), getCommonParameters()->getTrimMsMsPeaks(), getCommonParameters());

			// get datapoints to fit calibration function to
			Status(L"Acquiring calibration data points...", std::vector<std::wstring> {taskId, L"Individual Spectra Files"});
			DataPointAquisitionResults *acquisitionResults = nullptr;

			for (int i = 1; i <= 5; i++)
			{
				acquisitionResults = GetDataAcquisitionResults(myMsDataFile, originalUncalibratedFilePath, variableModifications, fixedModifications, proteinList, taskId, combinedParams, combinedParams->getPrecursorMassTolerance(), combinedParams->getProductMassTolerance());

				// enough data points to calibrate?
				if (acquisitionResults->Psms.size() >= NumRequiredPsms && acquisitionResults->getMs1List().size() > NumRequiredMs1Datapoints && acquisitionResults->getMs2List().size() > NumRequiredMs2Datapoints)
				{
					break;
				}

				if (i == 1) // failed round 1
				{
					PpmTolerance tempVar(20);
					getCommonParameters()->setPrecursorMassTolerance(&tempVar);
					PpmTolerance tempVar2(50);
					getCommonParameters()->setProductMassTolerance(&tempVar2);
				}
				else if (i == 2) // failed round 2
				{
					PpmTolerance tempVar3(30);
					getCommonParameters()->setPrecursorMassTolerance(&tempVar3);
					PpmTolerance tempVar4(100);
					getCommonParameters()->setProductMassTolerance(&tempVar4);
				}
				else if (i == 3) // failed round 3
				{
					PpmTolerance tempVar5(40);
					getCommonParameters()->setPrecursorMassTolerance(&tempVar5);
					PpmTolerance tempVar6(150);
					getCommonParameters()->setProductMassTolerance(&tempVar6);
				}
				else // failed round 4
				{
					if (acquisitionResults->Psms.size() < NumRequiredPsms)
					{
						Warn(L"Calibration failure! Could not find enough high-quality PSMs. Required " + std::to_wstring(NumRequiredPsms) + L", saw " + std::to_wstring(acquisitionResults->Psms.size()));
					}
					if (acquisitionResults->getMs1List().size() < NumRequiredMs1Datapoints)
					{
						Warn(L"Calibration failure! Could not find enough MS1 datapoints. Required " + std::to_wstring(NumRequiredMs1Datapoints) + L", saw " + std::to_wstring(acquisitionResults->getMs1List().size()));
					}
					if (acquisitionResults->getMs2List().size() < NumRequiredMs2Datapoints)
					{
						Warn(L"Calibration failure! Could not find enough MS2 datapoints. Required " + std::to_wstring(NumRequiredMs2Datapoints) + L", saw " + std::to_wstring(acquisitionResults->getMs2List().size()));
					}

					couldNotFindEnoughDatapoints = true;
					FinishedDataFile(originalUncalibratedFilePath, std::vector<std::wstring> {taskId, L"Individual Spectra Files", originalUncalibratedFilePath});
					break;
				}

				Warn(L"Could not find enough PSMs to calibrate with; opening up tolerances to " + std::round(getCommonParameters()->getPrecursorMassTolerance()->Value * std::pow(10, 2)) / std::pow(10, 2) + L" ppm precursor and " + std::round(getCommonParameters()->getProductMassTolerance()->Value * std::pow(10, 2)) / std::pow(10, 2) + L" ppm product");
			}

			if (couldNotFindEnoughDatapoints)
			{
				spectraFilesAfterCalibration.push_back(Path::GetFileNameWithoutExtension(currentRawFileList[spectraFileIndex]));
				ProgressEventArgs tempVar7(100, L"Failed to calibrate!", new std::vector<std::wstring> {taskId, L"Individual Spectra Files", originalUncalibratedFilenameWithoutExtension});
				ReportProgress(&tempVar7);
				continue;
			}

			// stats before calibration
			int prevPsmCount = acquisitionResults->Psms.size();
			double preCalibrationPrecursorErrorIqr = acquisitionResults->PsmPrecursorIqrPpmError;
			double preCalibrationProductErrorIqr = acquisitionResults->PsmProductIqrPpmError;

			// generate calibration function and shift data points
			Status(L"Calibrating...", std::vector<std::wstring> {taskId, L"Individual Spectra Files"});
			CalibrationEngine *engine = new CalibrationEngine(myMsDataFile, acquisitionResults, getCommonParameters(), std::vector<std::wstring> {taskId, L"Individual Spectra Files", originalUncalibratedFilenameWithoutExtension});
			engine->Run();

			//update file
			myMsDataFile = engine->getCalibratedDataFile();

			// do another search to evaluate calibration results
			Status(L"Post-calibration search...", std::vector<std::wstring> {taskId, L"Individual Spectra Files"});
			acquisitionResults = GetDataAcquisitionResults(myMsDataFile, originalUncalibratedFilePath, variableModifications, fixedModifications, proteinList, taskId, combinedParams, combinedParams->getPrecursorMassTolerance(), combinedParams->getProductMassTolerance());

			//generate calibration function and shift data points AGAIN because it's fast and contributes new data
			Status(L"Calibrating...", std::vector<std::wstring> {taskId, L"Individual Spectra Files"});
			engine = new CalibrationEngine(myMsDataFile, acquisitionResults, getCommonParameters(), std::vector<std::wstring> {taskId, L"Individual Spectra Files", originalUncalibratedFilenameWithoutExtension});
			engine->Run();

			//update file
			myMsDataFile = engine->getCalibratedDataFile();

			// write the calibrated mzML file
			MzmlMethods::CreateAndWriteMyMzmlWithCalibratedSpectra(myMsDataFile, calibratedFilePath, false);
			myFileManager->DoneWithFile(originalUncalibratedFilePath);

			// stats after calibration
			int postCalibrationPsmCount = acquisitionResults->Psms.size();
			double postCalibrationPrecursorErrorIqr = acquisitionResults->PsmPrecursorIqrPpmError;
			double postCalibrationProductErrorIqr = acquisitionResults->PsmProductIqrPpmError;

			// did the data improve? (not used for anything yet...)
			bool improvement = ImprovGlobal(preCalibrationPrecursorErrorIqr, preCalibrationProductErrorIqr, prevPsmCount, postCalibrationPsmCount, postCalibrationPrecursorErrorIqr, postCalibrationProductErrorIqr);

			// write toml settings for the calibrated file
			auto newTomlFileName = FileSystem::combine(OutputFolder, originalUncalibratedFilenameWithoutExtension + CalibSuffix + L".toml");

			auto fileSpecificParams = new FileSpecificParameters();

			// carry over file-specific parameters from the uncalibrated file to the calibrated one
			if (fileSettingsList[spectraFileIndex] != nullptr)
			{
				fileSpecificParams = fileSettingsList[spectraFileIndex]->Clone();
			}

			//suggest 4 * interquartile range as the ppm tolerance
			PpmTolerance tempVar8((4.0 * postCalibrationPrecursorErrorIqr) + std::abs(acquisitionResults->PsmPrecursorMedianPpmError));
			fileSpecificParams->setPrecursorMassTolerance(&tempVar8);
			PpmTolerance tempVar9((4.0 * postCalibrationProductErrorIqr) + std::abs(acquisitionResults->PsmProductMedianPpmError));
			fileSpecificParams->setProductMassTolerance(&tempVar9);

			Toml::WriteFile(fileSpecificParams, newTomlFileName, tomlConfig);

			FinishedWritingFile(newTomlFileName, std::vector<std::wstring> {taskId, L"Individual Spectra Files", originalUncalibratedFilenameWithoutExtension});

			// finished calibrating this file
			spectraFilesAfterCalibration.push_back(Path::GetFileNameWithoutExtension(calibratedFilePath));
			FinishedWritingFile(calibratedFilePath, std::vector<std::wstring> {taskId, L"Individual Spectra Files", originalUncalibratedFilenameWithoutExtension});
			MyTaskResults->NewSpectra.push_back(calibratedFilePath);
			MyTaskResults->NewFileSpecificTomls.push_back(newTomlFileName);
			FinishedDataFile(originalUncalibratedFilePath, std::vector<std::wstring> {taskId, L"Individual Spectra Files", originalUncalibratedFilePath});
			ProgressEventArgs tempVar10(100, L"Done!", new std::vector<std::wstring> {taskId, L"Individual Spectra Files", originalUncalibratedFilenameWithoutExtension});
			ReportProgress(&tempVar10);

//C# TO C++ CONVERTER TODO TASK: A 'delete fileSpecificParams' statement was not added since fileSpecificParams was passed to a method or constructor. Handle memory management manually.
			delete engine;
		}

		// re-write experimental design (if it has been defined) with new calibrated file names
		std::wstring assumedPathToExperDesign = Directory::GetParent(currentRawFileList.front())->FullName;
		assumedPathToExperDesign = FileSystem::combine(assumedPathToExperDesign, GlobalVariables::getExperimentalDesignFileName());

		if (FileSystem::fileExists(assumedPathToExperDesign))
		{
			WriteNewExperimentalDesignFile(assumedPathToExperDesign, OutputFolder, spectraFilesAfterCalibration);
		}

		// finished calibrating all files for the task
		ProgressEventArgs tempVar11(100, L"Done!", new std::vector<std::wstring> {taskId, L"Individual Spectra Files"});
		ReportProgress(&tempVar11);

		delete myFileManager;
		return MyTaskResults;
	}

const std::wstring CalibrationTask::CalibSuffix = L"-calib";

	bool CalibrationTask::ImprovGlobal(double prevPrecTol, double prevProdTol, int prevPsmCount, int thisRoundPsmCount, double thisRoundPrecTol, double thisRoundProdTol)
	{
		if (thisRoundPsmCount > prevPsmCount)
		{
			return true;
		}

		auto precRatio = thisRoundPrecTol / prevPrecTol;
		auto prodRatio = thisRoundProdTol / prevProdTol;

		if (thisRoundPsmCount == prevPsmCount)
		{
			return precRatio + prodRatio < 2; // Take any improvement in ratios
		}

		auto countRatio = static_cast<double>(thisRoundPsmCount) / prevPsmCount;
		return countRatio > 0.9 && precRatio + prodRatio < 1.8;
	}

	DataPointAquisitionResults *CalibrationTask::GetDataAcquisitionResults(MsDataFile *myMsDataFile, const std::wstring &currentDataFile, std::vector<Modification*> &variableModifications, std::vector<Modification*> &fixedModifications, std::vector<Protein*> &proteinList, const std::wstring &taskId, EngineLayer::CommonParameters *combinedParameters, Tolerance *initPrecTol, Tolerance *initProdTol)
	{
		auto fileNameWithoutExtension = Path::GetFileNameWithoutExtension(currentDataFile);
		SinglePpmAroundZeroSearchMode tempVar(initPrecTol->Value);
		MassDiffAcceptor *searchMode = dynamic_cast<PpmTolerance*>(initPrecTol) != nullptr ? static_cast<MassDiffAcceptor*>(&tempVar): new SingleAbsoluteAroundZeroSearchMode(initPrecTol->Value);

		auto listOfSortedms2Scans = GetMs2Scans(myMsDataFile, currentDataFile, combinedParameters).OrderBy([&] (std::any b)
		{
			b::PrecursorMass;
		})->ToArray();
		std::vector<PeptideSpectralMatch*> allPsmsArray(listOfSortedms2Scans.size());

		Log(L"Searching with searchMode: " + searchMode, std::vector<std::wstring> {taskId, L"Individual Spectra Files", fileNameWithoutExtension});
		Log(L"Searching with productMassTolerance: " + initProdTol, std::vector<std::wstring> {taskId, L"Individual Spectra Files", fileNameWithoutExtension});

		ClassicSearchEngine tempVar2(allPsmsArray, listOfSortedms2Scans, variableModifications, fixedModifications, proteinList, searchMode, combinedParameters, new std::vector<std::wstring> {taskId, L"Individual Spectra Files", fileNameWithoutExtension});
		(&tempVar2)->Run();
		std::vector<PeptideSpectralMatch*> allPsms = allPsmsArray.Where([&] (std::any b)
		{
			return b != nullptr;
		}).ToList();

		allPsms = allPsms.OrderByDescending([&] (std::any b)
		{
			b::Score;
		}).ThenBy([&] (std::any b)
		{
			b::PeptideMonisotopicMass.HasValue ? std::abs(b::ScanPrecursorMass - b::PeptideMonisotopicMass->Value) : std::numeric_limits<double>::max();
		}).GroupBy([&] (std::any b)
		{
			(b::FullFilePath, b::ScanNumber, b::PeptideMonisotopicMass);
		})->Select([&] (std::any b)
		{
			b::First();
		}).ToList();

		FdrAnalysisEngine tempVar3(allPsms, searchMode->NumNotches, getCommonParameters(), new std::vector<std::wstring> {taskId, L"Individual Spectra Files", fileNameWithoutExtension});
		(&tempVar3)->Run();

		std::vector<PeptideSpectralMatch*> goodIdentifications = allPsms.Where([&] (std::any b)
		{
			return b::FdrInfo::QValueNotch < 0.001 && !b::IsDecoy && b::FullSequence != nullptr;
		}).ToList();

		if (!goodIdentifications.Any())
		{
			return new DataPointAquisitionResults(nullptr, std::vector<PeptideSpectralMatch*>(), std::vector<LabeledDataPoint*>(), std::vector<LabeledDataPoint*>(), 0, 0, 0, 0);
		}

		DataPointAcquisitionEngine tempVar4(goodIdentifications, myMsDataFile, initPrecTol, getCalibrationParameters()->getMinMS1IsotopicPeaksNeededForConfirmedIdentification(), getCommonParameters(), new std::vector<std::wstring> {taskId, L"Individual Spectra Files", fileNameWithoutExtension});
		DataPointAquisitionResults *currentResult = static_cast<DataPointAquisitionResults*>((&tempVar4)->Run());

		return currentResult;
	}

	void CalibrationTask::WriteNewExperimentalDesignFile(const std::wstring &assumedPathToExperDesign, const std::wstring &outputFolder, std::vector<std::wstring> &spectraFilesAfterCalibration)
	{
		auto lines = File::ReadAllLines(assumedPathToExperDesign);
		std::vector<std::wstring> newExperimentalDesignOutput = std::vector<std::vector<std::wstring>>(0) };

		for (int i = 1; i < lines.size(); i++)
		{
			auto split = StringHelper::split(lines[i], L'\t');
			std::wstring oldFileName = Path::GetFileNameWithoutExtension(split[0]);
			std::wstring newFileName = oldFileName + CalibSuffix;
			std::wstring newline;

			if (!std::find(spectraFilesAfterCalibration.begin(), spectraFilesAfterCalibration.end(), newFileName) != spectraFilesAfterCalibration.end())
			{
				// file was not successfully calibrated
				newline = oldFileName + L"\t";
			}
			else
			{
				// file was successfully calibrated
				newline = newFileName + L"\t";
			}

			// add condition, biorep, etc info
			for (int j = 1; j < split.size(); j++)
			{
				newline += split[j] + L"\t";
			}

			// write the line
			newExperimentalDesignOutput.push_back(newline);
		}

		File::WriteAllLines(FileSystem::combine(outputFolder, GlobalVariables::getExperimentalDesignFileName()), newExperimentalDesignOutput);
	}
}
