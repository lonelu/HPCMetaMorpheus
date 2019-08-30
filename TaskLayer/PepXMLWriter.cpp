﻿#include "PepXMLWriter.h"
#include "../EngineLayer/CommonParameters.h"
#include "../EngineLayer/PeptideSpectralMatch.h"
#include "DbForTask.h"
#include "../EngineLayer/GlobalVariables.h"

using namespace EngineLayer;
using namespace Proteomics;
using namespace Proteomics::Fragmentation;
using namespace Proteomics::ProteolyticDigestion;

namespace TaskLayer
{

	void PepXMLWriter::WritePepXml(std::vector<PeptideSpectralMatch*> &psms, std::vector<DbForTask*> &database, std::vector<Modification*> &variableModifications, std::vector<Modification*> &fixedModifications, CommonParameters *CommonParameters, const std::string &outputPath, double qValueFilter)
	{
		// TODO: needs a unit test
		psms = psms.Where([&] (std::any p)
		{
			return p::FdrInfo::QValue <= qValueFilter && p::FdrInfo::QValueNotch < qValueFilter;
		}).ToList();

		if (!psms.Any())
		{
			return;
		}

		XmlSerializer *_indexedSerializer = new XmlSerializer(pepXML::Generated::msms_pipeline_analysis::typeid);
		auto _pepxml = new pepXML::Generated::msms_pipeline_analysis();

		_pepxml->date = DateTime::Now;
		_pepxml->summary_xml = psms[0]->getFullFilePath() + L".pep.XML";

		std::string proteaseNC = std::string::Join("", CommonParameters->getDigestionParams()->Protease.DigestionMotifs->Select([&] (std::any m)
		{
			m::InducingCleavage;
		}));
		std::string proteaseC = std::string::Join("", CommonParameters->getDigestionParams()->Protease.DigestionMotifs->Select([&] (std::any m)
		{
			m::InducingCleavage;
		}));

		std::string fileNameNoExtension = Path::GetFileNameWithoutExtension(psms[0]->getFullFilePath());
		std::string filePathNoExtension = Path::ChangeExtension(psms[0]->getFullFilePath(), "");

		auto para = std::vector<pepXML::Generated::nameValueType*>();
		{
			pepXML::Generated::nameValueType *tempVar = new pepXML::Generated::nameValueType();
			tempVar->name = "threads";
			tempVar->value = std::to_string(CommonParameters->getMaxThreadsToUsePerFile());
			para.push_back(tempVar);
			pepXML::Generated::nameValueType *tempVar2 = new pepXML::Generated::nameValueType();
			tempVar2->name = "database";
			tempVar2->value = database.front().FilePath;
			para.push_back(tempVar2);
			pepXML::Generated::nameValueType *tempVar3 = new pepXML::Generated::nameValueType();
			tempVar3->name = "MS_data_file";
			tempVar3->value = psms[0]->getFullFilePath();
			para.push_back(tempVar3);

			pepXML::Generated::nameValueType *tempVar4 = new pepXML::Generated::nameValueType();
			tempVar4->name = "MaxMissed Cleavages";

			tempVar4->value = CommonParameters->getDigestionParams()->MaxMissedCleavages.ToString();
			para.push_back(tempVar4);
			pepXML::Generated::nameValueType *tempVar5 = new pepXML::Generated::nameValueType();
			tempVar5->name = "Protease";
			tempVar5->value = CommonParameters->getDigestionParams()->Protease->Name;
			para.push_back(tempVar5);
			pepXML::Generated::nameValueType *tempVar6 = new pepXML::Generated::nameValueType();
			tempVar6->name = "Initiator Methionine";

			tempVar6->value = CommonParameters->getDigestionParams()->InitiatorMethionineBehavior.ToString();
			para.push_back(tempVar6);
			pepXML::Generated::nameValueType *tempVar7 = new pepXML::Generated::nameValueType();
			tempVar7->name = "Max Modification Isoforms";

			tempVar7->value = CommonParameters->getDigestionParams()->MaxModificationIsoforms.ToString();
			para.push_back(tempVar7);
			pepXML::Generated::nameValueType *tempVar8 = new pepXML::Generated::nameValueType();
			tempVar8->name = "Min Peptide Len";

			tempVar8->value = CommonParameters->getDigestionParams()->MinPeptideLength.ToString();
			para.push_back(tempVar8);
			pepXML::Generated::nameValueType *tempVar9 = new pepXML::Generated::nameValueType();
			tempVar9->name = "Max Peptide Len";

			tempVar9->value = CommonParameters->getDigestionParams()->MaxPeptideLength.ToString();
			para.push_back(tempVar9);
			pepXML::Generated::nameValueType *tempVar10 = new pepXML::Generated::nameValueType();
			tempVar10->name = "Product Mass Tolerance";

			tempVar10->value = CommonParameters->getProductMassTolerance()->ToString();
			para.push_back(tempVar10);
			// TODO: check this
			pepXML::Generated::nameValueType *tempVar11 = new pepXML::Generated::nameValueType();
			tempVar11->name = "Ions to search";
			tempVar11->value = std::string::Join(", ", DissociationTypeCollection::ProductsFromDissociationType[CommonParameters->getDissociationType()]);
			para.push_back(tempVar11);
			pepXML::Generated::nameValueType *tempVar12 = new pepXML::Generated::nameValueType();
			tempVar12->name = "Q-value Filter";
			tempVar12->value = std::to_string(CommonParameters->getQValueOutputFilter());
			para.push_back(tempVar12);
			for (auto item : fixedModifications)
			{
				pepXML::Generated::nameValueType *tempVar13 = new pepXML::Generated::nameValueType();
				tempVar13->name = "Fixed Modifications: " + item->IdWithMotif;

				tempVar13->value = item->MonoisotopicMass.ToString();
				para.push_back(tempVar13);
			}
			for (auto item : variableModifications)
			{
				pepXML::Generated::nameValueType *tempVar14 = new pepXML::Generated::nameValueType();
				tempVar14->name = "Variable Modifications: " + item->IdWithMotif;

				tempVar14->value = item->MonoisotopicMass.ToString();
				para.push_back(tempVar14);
			}

			pepXML::Generated::nameValueType *tempVar15 = new pepXML::Generated::nameValueType();
			tempVar15->name = "Localize All Modifications";
			tempVar15->value = "true";
			para.push_back(tempVar15);
		}

		pepXML::Generated::msms_pipeline_analysisMsms_run_summary *tempVar16 = new pepXML::Generated::msms_pipeline_analysisMsms_run_summary();
		tempVar16->base_name = filePathNoExtension;
		tempVar16->raw_data_type = "raw";
		tempVar16->raw_data = ".mzM";
		tempVar16->sample_enzyme = new pepXML::Generated::msms_pipeline_analysisMsms_run_summarySample_enzyme();
		tempVar16->sample_enzyme->name = CommonParameters->getDigestionParams()->Protease->Name;
		pepXML::Generated::msms_pipeline_analysisMsms_run_summarySample_enzymeSpecificity *tempVar17 = new pepXML::Generated::msms_pipeline_analysisMsms_run_summarySample_enzymeSpecificity();
		tempVar17->cut = proteaseC;
		tempVar17->no_cut = proteaseNC;
		tempVar16->sample_enzyme->specificity = {tempVar17};
		pepXML::Generated::msms_pipeline_analysisMsms_run_summarySearch_summary *tempVar18 = new pepXML::Generated::msms_pipeline_analysisMsms_run_summarySearch_summary();
		tempVar18->base_name = filePathNoExtension;
		tempVar18->search_engine_version = GlobalVariables::getMetaMorpheusVersion();
		tempVar18->precursor_mass_type = pepXML::Generated::massType::monoisotopic;
		tempVar18->fragment_mass_type = pepXML::Generated::massType::monoisotopic;
		tempVar18->search_id = 1;
		tempVar18->search_database = new pepXML::Generated::msms_pipeline_analysisMsms_run_summarySearch_summarySearch_database();
		tempVar18->search_database->local_path = database.front().FilePath;
		tempVar18->search_database->type = pepXML::Generated::msms_pipeline_analysisMsms_run_summarySearch_summarySearch_databaseType::AA;
		tempVar18->enzymatic_search_constraint = new pepXML::Generated::msms_pipeline_analysisMsms_run_summarySearch_summaryEnzymatic_search_constraint();
		tempVar18->enzymatic_search_constraint->enzyme = CommonParameters->getDigestionParams()->Protease->Name;

		tempVar18->enzymatic_search_constraint->max_num_internal_cleavages = CommonParameters->getDigestionParams()->MaxMissedCleavages.ToString();
		tempVar18->parameter = para.ToArray();
		tempVar16->search_summary = {tempVar18};
		_pepxml->msms_run_summary = {tempVar16};

		_pepxml->msms_run_summary[0]->spectrum_query = std::vector<pepXML::Generated::msms_pipeline_analysisMsms_run_summarySpectrum_query*>(psms.size());

		auto searchHits = std::vector<pepXML::Generated::msms_pipeline_analysisMsms_run_summarySpectrum_querySearch_resultSearch_hit*>();

		for (auto psm : psms)
		{
			PeptideWithSetModifications *peptide = psm->BestMatchingPeptides.First().Peptide;

			auto mods = std::vector<pepXML::Generated::modInfoDataTypeMod_aminoacid_mass*>();
			for (auto mod : peptide->AllModsOneIsNterminus)
			{
				auto pepXmlMod = new pepXML::Generated::modInfoDataTypeMod_aminoacid_mass();
				pepXmlMod->mass = static_cast<double>(mod->Value->MonoisotopicMass);

				pepXmlMod->position = (mod->Key - 1).ToString();
				mods.push_back(pepXmlMod);

//C# TO C++ CONVERTER TODO TASK: A 'delete pepXmlMod' statement was not added since pepXmlMod was passed to a method or constructor. Handle memory management manually.
			}

			auto proteinAccessions = psm->BestMatchingPeptides->Select([&] (std::any p)
			{
				p::Peptide::Protein::Accession;
			}).Distinct();

			auto searchHit = new pepXML::Generated::msms_pipeline_analysisMsms_run_summarySpectrum_querySearch_resultSearch_hit();
			searchHit->hit_rank = 1;
			searchHit->peptide = ((psm->getBaseSequence() != "") ? psm->getBaseSequence() : "Ambiguous");

			searchHit->peptide_prev_aa = peptide->PreviousAminoAcid.ToString();

			searchHit->peptide_next_aa = peptide->NextAminoAcid.ToString();
			searchHit->protein = ((peptide->Protein.Accession != nullptr) ? peptide->Protein.Accession : std::string::Join("|", proteinAccessions));
			searchHit->num_tot_proteins = static_cast<unsigned int>(proteinAccessions->Count());
			searchHit->calc_neutral_pep_mass = static_cast<float>((psm->getPeptideMonisotopicMass() != nullptr) ? psm->getPeptideMonisotopicMass() : NAN);
			searchHit->massdiff = ((psm->getPeptideMonisotopicMass() != nullptr) ? std::to_string(psm->getScanPrecursorMass() - psm->getPeptideMonisotopicMass().Value) : "Ambiguous");
			pepXML::Generated::modInfoDataType *tempVar19 = new pepXML::Generated::modInfoDataType();
			tempVar19->mod_aminoacid_mass = mods.ToArray();
			searchHit->modification_info = (mods.empty() ? tempVar19 : nullptr);
			pepXML::Generated::nameValueType *tempVar20 = new pepXML::Generated::nameValueType();
			tempVar20->name = "Score";
			tempVar20->value = std::to_string(psm->getScore());
			pepXML::Generated::nameValueType *tempVar21 = new pepXML::Generated::nameValueType();
			tempVar21->name = "Qvalue";
			tempVar21->value = std::to_string(psm->getFdrInfo()->getQValue());
			searchHit->search_score = {tempVar20, tempVar21};
			searchHits.push_back(searchHit);

//C# TO C++ CONVERTER TODO TASK: A 'delete searchHit' statement was not added since searchHit was passed to a method or constructor. Handle memory management manually.
		}

		for (int i = 0; i < psms.size(); i++)
		{
			pepXML::Generated::msms_pipeline_analysisMsms_run_summarySpectrum_query *tempVar22 = new pepXML::Generated::msms_pipeline_analysisMsms_run_summarySpectrum_query();

			tempVar22->spectrum = fileNameNoExtension + "." + psms[i]->getScanNumber().ToString();
			tempVar22->start_scan = static_cast<unsigned int>(psms[i]->getScanNumber());
			tempVar22->end_scan = static_cast<unsigned int>(psms[i]->getScanNumber());
			tempVar22->precursor_neutral_mass = static_cast<float>(psms[i]->getScanPrecursorMass());

			tempVar22->assumed_charge = psms[i]->getScanPrecursorCharge().ToString();
			tempVar22->index = static_cast<unsigned int>(i + 1);
			tempVar22->retention_time_sec = static_cast<float>(psms[i]->getScanRetentionTime() * 60);
			pepXML::Generated::msms_pipeline_analysisMsms_run_summarySpectrum_querySearch_result *tempVar23 = new pepXML::Generated::msms_pipeline_analysisMsms_run_summarySpectrum_querySearch_result();
			tempVar23->search_hit = {searchHits[i]};
			tempVar22->search_result = {tempVar23};
			_pepxml->msms_run_summary[0].spectrum_query[i] = tempVar22;
		}

		TextWriter *writer = new StreamWriter(FileSystem::combine(outputPath));
		_indexedSerializer->Serialize(writer, _pepxml);
		writer->Close();

//C# TO C++ CONVERTER TODO TASK: A 'delete writer' statement was not added since writer was passed to a method or constructor. Handle memory management manually.
//C# TO C++ CONVERTER TODO TASK: A 'delete _pepxml' statement was not added since _pepxml was passed to a method or constructor. Handle memory management manually.
		delete _indexedSerializer;
	}
}
