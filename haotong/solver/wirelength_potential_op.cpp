#include "solverObject.h"
#include "wirelength_potential_op.h"

RecSteinerMinTree rsmt;

// 添加缓存类定义
class RsmtCache {
private:
    // 缓存键结构
    struct CacheKey {
        std::vector<int> x_coords;
        std::vector<int> y_coords;
        
        bool operator==(const CacheKey& other) const {
            return x_coords == other.x_coords && 
                   y_coords == other.y_coords;
        }
    };
    
    // 哈希函数
    struct KeyHasher {
        std::size_t operator()(const CacheKey& key) const {
            std::size_t hash = 0;
            for(size_t i = 0; i < key.x_coords.size(); ++i) {
                hash ^= std::hash<int>{}(key.x_coords[i]) + 
                        std::hash<int>{}(key.y_coords[i]) + 
                        0x9e3779b9 + (hash << 6) + (hash >> 2);
            }
            return hash;
        }
    };

    // 缓存数据结构
    std::unordered_map<CacheKey, int, KeyHasher> wire_length_cache;
    const size_t MAX_CACHE_SIZE = 10000;

public:
    int getWireLength(const std::vector<int>& x, const std::vector<int>& y) {
        // 对于2个pin的情况直接计算
        if(x.size() == 2) {
            return std::abs(x[0] - x[1]) + std::abs(y[0] - y[1]);
        }
        
        // 构造缓存键并查找
        CacheKey key{x, y};
        auto it = wire_length_cache.find(key);
        if(it != wire_length_cache.end()) {
            return it->second;
        }
        
        // 缓存未命中，计算RSMT
        Tree mst = rsmt.fltTree(x, y);
        int wire_length = rsmt.wirelength(mst);
        
        // 检查缓存大小并存储
        if(wire_length_cache.size() >= MAX_CACHE_SIZE) {
            wire_length_cache.clear(); // 简单的清理策略
        }
        wire_length_cache[key] = wire_length;
        return wire_length;
    }
};

// 修改主函数
int calculate_WL_Increase(SInstance* inst_old_ptr, std::tuple<int, int, int> newLoc) {
    static RsmtCache rsmt_cache; // 静态缓存实例
    
    int result = 0;
    std::set<int> netID_set;
    for (auto inpin : inst_old_ptr->inpins) {
        if (inpin->netID != -1) netID_set.insert(inpin->netID);
    }
    for (auto outpin : inst_old_ptr->outpins) {
        if (outpin->netID != -1) netID_set.insert(outpin->netID);
    }

    for (int netID : netID_set) {
        assert(netID >= 0 && netID < int(NetArray.size()));
        const SNet* net = NetArray[netID];

        // crit wirelength calculation (保持不变)
        int crit_prev = 0;
        int crit_new = 0;
        const SPin* driverPin = net->inpin;
        if (!driverPin) continue;

        std::tuple<int, int, int> driverLoc = driverPin->instanceOwner->Location;
        // Don't forget to modify the driver location!
        std::tuple<int, int, int> driverLoc_new;
        if (driverPin->instanceOwner == inst_old_ptr) {
            driverLoc_new = newLoc;
        }
        else {
            driverLoc_new = driverLoc;
        }
        std::set<std::pair<int, int>> mergedPinLocs_prev{};
        std::set<std::pair<int, int>> mergedPinLocs_new{};
        for (auto outpin : net->outpins) {
            if (!outpin->timingCritical) continue;
            std::tuple<int, int, int> sinkLoc = outpin->instanceOwner->Location;
            mergedPinLocs_prev.insert(std::make_pair(std::get<0>(sinkLoc), std::get<1>(sinkLoc)));
            if (outpin->instanceOwner == inst_old_ptr) {
                assert(outpin->instanceOwner->id == inst_old_ptr->id);
                mergedPinLocs_new.insert(std::make_pair(std::get<0>(newLoc), std::get<1>(newLoc)));
            }
            else {
                mergedPinLocs_new.insert(std::make_pair(std::get<0>(sinkLoc), std::get<1>(sinkLoc)));
            }
        }
        for (auto loc : mergedPinLocs_prev) {
            crit_prev += std::abs(std::get<0>(loc) - std::get<0>(driverLoc)) + std::abs(std::get<1>(loc) - std::get<1>(driverLoc));
        }
        for (auto loc : mergedPinLocs_new) {
            crit_new += std::abs(std::get<0>(loc) - std::get<0>(driverLoc_new)) + std::abs(std::get<1>(loc) - std::get<1>(driverLoc_new));
        }
        // 更优化地，可以直接只放入更新的pin
        result += (crit_new - crit_prev) * 2;
        
        // non-crit wirelength calculation (使用缓存)
        int noncrit_prev = 0;
        int noncrit_new = 0;
        std::set<std::pair<int, int>> rsmtPinLocs_prev{};
        std::set<std::pair<int, int>> rsmtPinLocs_new{};
        
        // 收集pin位置（保持原有代码不变）
        rsmtPinLocs_prev.insert(std::make_pair(std::get<0>(driverLoc), std::get<1>(driverLoc)));
        rsmtPinLocs_new.insert(std::make_pair(std::get<0>(driverLoc_new), std::get<1>(driverLoc_new)));
        
        for (auto outpin : net->outpins) {
            if (outpin->timingCritical) continue;
            std::tuple<int, int, int> sinkLoc = outpin->instanceOwner->Location;
            rsmtPinLocs_prev.insert(std::make_pair(std::get<0>(sinkLoc), std::get<1>(sinkLoc)));
            if (outpin->instanceOwner == inst_old_ptr) {
                assert(outpin->instanceOwner->id == inst_old_ptr->id);
                rsmtPinLocs_new.insert(std::make_pair(std::get<0>(newLoc), std::get<1>(newLoc)));
            }
            else {
                rsmtPinLocs_new.insert(std::make_pair(std::get<0>(sinkLoc), std::get<1>(sinkLoc)));
            }
        }

        // 使用vector存储坐标
        std::vector<int> xCoords_prev, yCoords_prev;
        std::vector<int> xCoords_new, yCoords_new;
        
        // 预分配内存
        xCoords_prev.reserve(rsmtPinLocs_prev.size());
        yCoords_prev.reserve(rsmtPinLocs_prev.size());
        xCoords_new.reserve(rsmtPinLocs_new.size());
        yCoords_new.reserve(rsmtPinLocs_new.size());

        for (auto loc : rsmtPinLocs_prev) {
            xCoords_prev.push_back(loc.first);
            yCoords_prev.push_back(loc.second);
        }
        for (auto loc : rsmtPinLocs_new) {
            xCoords_new.push_back(loc.first);
            yCoords_new.push_back(loc.second);
        }

        // 使用缓存计算RSMT
        if (xCoords_prev.size() > 1) {
            noncrit_prev = rsmt_cache.getWireLength(xCoords_prev, yCoords_prev);
        }
        if (xCoords_new.size() > 1) {
            noncrit_new = rsmt_cache.getWireLength(xCoords_new, yCoords_new);
        }

        result += noncrit_new - noncrit_prev;
    }
    return result;
}


// int calculate_WL_Increase(SInstance* inst_old_ptr, std::tuple<int, int, int> newLoc) {
//     // calculate the previous wirelength
//     int result = 0;
//     // std::cout << "inst_old_ptr->id: " << inst_old_ptr->id << std::endl;
//     // if (inst_old_ptr->id == 1970) std::cout << "newLoc: " << std::get<0>(newLoc) << " " << std::get<1>(newLoc) << " " << std::get<2>(newLoc) << std::endl;
//     std::set<int> netID_set;
//     for (auto inpin : inst_old_ptr->inpins) {
//         if (inpin->netID != -1) netID_set.insert(inpin->netID);
//     }
//     for (auto outpin : inst_old_ptr->outpins) {
//         if (outpin->netID != -1) netID_set.insert(outpin->netID);
//     }
//     for (int netID : netID_set) {
//         // std::cout << "netID: " << netID << std::endl;
//         assert(netID >= 0 && netID < int(NetArray.size()));
//         const SNet* net = NetArray[netID];

//         // crit wirelength of this net
//         int crit_prev = 0;
//         int crit_new = 0;
//         const SPin* driverPin = net->inpin;
//         if (!driverPin) continue;

//         std::tuple<int, int, int> driverLoc = driverPin->instanceOwner->Location;
//         // Don't forget to modify the driver location!
//         std::tuple<int, int, int> driverLoc_new;
//         if (driverPin->instanceOwner == inst_old_ptr) {
//             driverLoc_new = newLoc;
//         }
//         else {
//             driverLoc_new = driverLoc;
//         }
//         std::set<std::pair<int, int>> mergedPinLocs_prev{};
//         std::set<std::pair<int, int>> mergedPinLocs_new{};
//         for (auto outpin : net->outpins) {
//             if (!outpin->timingCritical) continue;
//             std::tuple<int, int, int> sinkLoc = outpin->instanceOwner->Location;
//             mergedPinLocs_prev.insert(std::make_pair(std::get<0>(sinkLoc), std::get<1>(sinkLoc)));
//             if (outpin->instanceOwner == inst_old_ptr) {
//                 assert(outpin->instanceOwner->id == inst_old_ptr->id);
//                 mergedPinLocs_new.insert(std::make_pair(std::get<0>(newLoc), std::get<1>(newLoc)));
//             }
//             else {
//                 mergedPinLocs_new.insert(std::make_pair(std::get<0>(sinkLoc), std::get<1>(sinkLoc)));
//             }
//         }
//         for (auto loc : mergedPinLocs_prev) {
//             crit_prev += std::abs(std::get<0>(loc) - std::get<0>(driverLoc)) + std::abs(std::get<1>(loc) - std::get<1>(driverLoc));
//         }
//         for (auto loc : mergedPinLocs_new) {
//             crit_new += std::abs(std::get<0>(loc) - std::get<0>(driverLoc_new)) + std::abs(std::get<1>(loc) - std::get<1>(driverLoc_new));
//         }
//         // 更优化地，可以直接只放入更新的pin
//         result += (crit_new - crit_prev) * 2;
//         // if (inst_old_ptr->id == 1970) std::cout << "crit_new: " << crit_new << " crit_prev: " << crit_prev << std::endl;



//         // 对于关键路径的计算优化其实不用很多
//         ////////////////////////////////////////////////////////////////////////
//         // non-crit wirelength of this net
//         int noncrit_prev = 0;
//         int noncrit_new = 0;
//         std::set<std::pair<int, int>> rsmtPinLocs_prev{};
//         std::set<std::pair<int, int>> rsmtPinLocs_new{};
//         rsmtPinLocs_prev.insert(std::make_pair(std::get<0>(driverLoc), std::get<1>(driverLoc)));
//         rsmtPinLocs_new.insert(std::make_pair(std::get<0>(driverLoc_new), std::get<1>(driverLoc_new)));
//         for (auto outpin : net->outpins) {
//             if (outpin->timingCritical) continue;
//             std::tuple<int, int, int> sinkLoc = outpin->instanceOwner->Location;
//             rsmtPinLocs_prev.insert(std::make_pair(std::get<0>(sinkLoc), std::get<1>(sinkLoc)));
//             if (outpin->instanceOwner == inst_old_ptr) {
//                 assert(outpin->instanceOwner->id == inst_old_ptr->id);
//                 rsmtPinLocs_new.insert(std::make_pair(std::get<0>(newLoc), std::get<1>(newLoc)));
//             }
//             else {
//                 rsmtPinLocs_new.insert(std::make_pair(std::get<0>(sinkLoc), std::get<1>(sinkLoc)));
//             }
//         }
//         std::vector<int> xCoords_prev, yCoords_prev;
//         std::vector<int> xCoords_new, yCoords_new;
//         for (auto loc : rsmtPinLocs_prev) {
//             xCoords_prev.push_back(loc.first);
//             yCoords_prev.push_back(loc.second);
//         }
//         for (auto loc : rsmtPinLocs_new) {
//             xCoords_new.push_back(loc.first);
//             yCoords_new.push_back(loc.second);
//         }
//         if (xCoords_prev.size() > 1) {
//             // 对于2-3个pin的情况，可以直接计算而不需要调用FLUTE
//             if (xCoords_prev.size() == 2) {
//                 noncrit_prev = std::abs(xCoords_prev[0] - xCoords_prev[1]) + 
//                             std::abs(yCoords_prev[0] - yCoords_prev[1]);
//             } else if (xCoords_prev.size() > 2) {
//                 Tree mst_prev = rsmt.fltTree(xCoords_prev, yCoords_prev);
//                 noncrit_prev = rsmt.wirelength(mst_prev);
//             }
//         }
//         if (xCoords_new.size() > 1) {
//             if (xCoords_new.size() == 2) {
//                 noncrit_new = std::abs(xCoords_new[0] - xCoords_new[1]) + 
//                             std::abs(yCoords_new[0] - yCoords_new[1]);
//             } else if (xCoords_new.size() > 2) {
//                 Tree mst_new = rsmt.fltTree(xCoords_new, yCoords_new);
//                 noncrit_new = rsmt.wirelength(mst_new);
//             }
//         }
//         result += noncrit_new - noncrit_prev;
//         // if (inst_old_ptr->id == 1970) std::cout << "noncrit_new: " << noncrit_new << " noncrit_prev: " << noncrit_prev << std::endl;
//     }
//     return result;
// }