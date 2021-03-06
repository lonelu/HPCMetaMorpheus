﻿#pragma once

#include "../MetaMorpheusEngineResults.h"
#include <string>
#include <vector>
#include "stringbuilder.h"

//C# TO C++ CONVERTER NOTE: Forward class declarations:
namespace EngineLayer { namespace Indexing { class IndexingEngine; } }

#include "Proteomics/Fragmentation/Fragmentation.h"
using namespace Proteomics::Fragmentation;

#include "Proteomics/ProteolyticDigestion/PeptideWithSetModifications.h"
using namespace Proteomics::ProteolyticDigestion;

namespace EngineLayer
{
    namespace Indexing
    {
        class IndexingResults : public MetaMorpheusEngineResults
        {
        private:
            std::vector<std::vector<int>> privateFragmentIndex;
            std::vector<std::vector<int>> privatePrecursorIndex;
            std::vector<PeptideWithSetModifications*> privatePeptideIndex;
            
        public:
            IndexingResults(std::vector<PeptideWithSetModifications*> &peptideIndex,
                            std::vector<std::vector<int>> &fragmentIndex,
                            std::vector<std::vector<int>> &precursorIndex,
                            IndexingEngine *indexParams);

            IndexingResults(IndexingEngine *indexParams);

            
            std::vector<std::vector<int>> &getFragmentIndex();
            void setFragmentIndex(const std::vector<std::vector<int>> &value);
            std::vector<std::vector<int>> &getPrecursorIndex();
            void setPrecursorIndex(const std::vector<std::vector<int>> &value);
            std::vector<PeptideWithSetModifications*> &getPeptideIndex();
            void setPeptideIndex(const std::vector<PeptideWithSetModifications*> &value);
            
            std::string ToString();
        };
    }
}
