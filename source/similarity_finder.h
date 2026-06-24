#ifndef RME_SIMILARITY_FINDER_H_
#define RME_SIMILARITY_FINDER_H_

#include "items.h"
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
#include <span>

namespace RME::Utils {

class SimilarityFinder {
public:
    static double CalculateSimilarity(const ItemType& a, const ItemType& b) noexcept {
        double score = 0.0;
        
        // 1. Name Similarity (Levenshtein distance or substring match)
        std::string nameA = a.name;
        std::string nameB = b.name;
        std::transform(nameA.begin(), nameA.end(), nameA.begin(), ::tolower);
        std::transform(nameB.begin(), nameB.end(), nameB.begin(), ::tolower);
        
        if (nameA == nameB) {
            score += 50.0;
        } else if (!nameA.empty() && !nameB.empty()) {
            if (nameA.find(nameB) != std::string::npos || nameB.find(nameA) != std::string::npos) {
                score += 30.0;
            }
        }
        
        // 2. Type/Attributes match
        if (a.type == b.type) score += 20.0;
        if (a.alwaysOnTopOrder == b.alwaysOnTopOrder) score += 10.0;
        if (a.isGroundTile() == b.isGroundTile()) score += 15.0;
        if (a.isBorder == b.isBorder) score += 15.0;
        if (a.stackable == b.stackable) score += 10.0;
        
        return score;
    }

    static std::vector<uint16_t> FindSimilarItems(uint16_t itemid, const ItemDatabase& db, size_t max_results = 5) {
        std::vector<std::pair<uint16_t, double>> candidates;
        const ItemType& source = db.getItemType(itemid);
        if (source.id == 0) return {};

        for (uint16_t id = 1; id < 30000; ++id) {
            if (id == itemid) continue;
            const ItemType& target = db.getItemType(id);
            if (target.id == 0 || target.name.empty()) continue;

            double score = CalculateSimilarity(source, target);
            if (score > 30.0) {
                candidates.push_back({id, score});
            }
        }

        std::sort(candidates.begin(), candidates.end(), [](const auto& l, const auto& r) {
            return l.second > r.second;
        });

        std::vector<uint16_t> results;
        for (size_t i = 0; i < std::min(max_results, candidates.size()); ++i) {
            results.push_back(candidates[i].first);
        }
        return results;
    }
};

} // namespace RME::Utils

#endif
