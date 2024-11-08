#include <vector>
#include <tuple>

#define MAXN 1e9

// the basic structure
struct Instance
{
    std::tuple<int, int, int> Location;
    int site_id;
    int type;
    int numMov = 0;
    std::vector<int> conn; // * 相当于连边，得另外建 - fixed的元件，不要加进conn里面?
    std::vector<int> mate;
    std::vector<int> pin_ids;
};
struct Net
{
    std::vector<int> pins_id;
    std::vector<int> bbox;
};
struct Pin
{
    int inst_id;
    int net_id;
    int offset; // 他的offset疑似是两个数，不知道为什么
};
std::vector<Instance> InstArray;
std::vector<Net> NetArray;
std::vector<Pin> PinArray;

struct IndepSet
{
    std::vector<int> set;        // 在这个独立集里面的instances
    std::vector<bool> dep;       // 每个对应的instance是不是独立的，独立为1，非独立为0 
    bool finish;                // 是否完成
    std::pair<int,int> ceSet;    // CE pair待定
    int cksr = -1;               // Clock/Reset signal 待定
    int type;
};


void runCLBISM()
{
    // initialize the ISM problems
    std::vector<std::vector<int>> cluster;
    // IndexVector = std::vector<IndexType>;

    // what is CLB ISM Problem?
    
    // initialize: netlist


    // initialize instance (loc, type, assignment?), pin offsets

    // perform ISM
    int xl = 0, yl = 0; // * lower left corner
    // slr_aware_flag = false, not considered

    // build ISM problem
    // > build Netlist
        // copy inst informations
        // copy net informations
        // copy pin informations
    // > build SiteMap
        // set instance into site
        // set control set
    // sort and remove redundancy

    // allocate memory for ISM solver
    // initialize seeds, related to priorty
    std::vector<int> priority;
    std::vector<std::vector<int>> movBuckets;
    for (int i = 0; i < InstArray.size(); ++i)
    {
        const auto &inst = InstArray[i];
        if (inst.type != -1)
        {
            priority.push_back(i);
        }
    }
    // compute HPWL，但是我们的实现中想必是调FLUTE去解的
    for (auto &net : NetArray)
    {
        net.bbox[0] = MAXN;
        net.bbox[1] = MAXN;
        net.bbox[2] = -MAXN;
        net.bbox[3] = -MAXN;
        // xl-0, yl-1, xh-2, yh-3
        for (int pid : net.pins_id)
        {
            const auto &pin = PinArray[pid];
            auto loc = InstArray[pin.inst_id].Location;
            // net.bbox.encompass(loc + pin.offset);
            // 因为bb写了int，没法加offset在上面
            net.bbox[0] = std::min(net.bbox[0], std::get<0>(loc));
            net.bbox[1] = std::min(net.bbox[1], std::get<1>(loc));
            net.bbox[2] = std::max(net.bbox[2], std::get<0>(loc));
            net.bbox[3] = std::max(net.bbox[3], std::get<1>(loc));
        }
    }
    // maxNetDegree = 16
    // openparf里长和宽还有权重，这里就不写了
    // 赏析 16 在此处的作用
    int HPWL_res = 0;
    for (const auto &net : NetArray)
    {
        if (net.pins_id.size() <= 16)
        {
            HPWL_res += (net.bbox[2] - net.bbox[0]) + (net.bbox[3] - net.bbox[1]);
        }
    }


    // main iteration: solver.run()
    int iteration = 0;
    while (true)
    {
        ++iteration;
        // the check of stop condition
        int HPWL_res_new = 0;
        for (const auto &net : NetArray)
        {
            if (net.pins_id.size() <= 16)
            {
                HPWL_res += (net.bbox[2] - net.bbox[0]) + (net.bbox[3] - net.bbox[1]);
            }
        }
        double improv = 1.0 - HPWL_res_new / HPWL_res;
        if (improv < 0.001) break;
        else HPWL_res = HPWL_res_new;

        std::vector<std::vector<int>> indepSets;
        IndepSet indepSet_builder; // "helper object"
        std::vector<std::vector<int>> sets;

        // build independent sets -------------------
            // sort priority by numMov, update priority
        for (auto &bkt : movBuckets)
            bkt.clear();
        int maxMove = 0;
        for (int i : priority)
        {
            maxMove = std::max(maxMove, InstArray[i].numMov);
        }
        movBuckets.resize(maxMove + 1);
        for (int i : priority)
        {
            movBuckets[InstArray[i].numMov].push_back(i);
        }
        auto it = priority.begin();
        for (const auto &bkt : movBuckets)
        {
            std::copy(bkt.begin(), bkt.end(), it);
            it += bkt.size();
        }

        indepSet_builder.dep.resize(InstArray.size(), 0);
        sets.clear();

        for (int inst_id : priority)
        {
            if (!indepSet_builder.dep[inst_id])
            {
                indepSet_builder.set.clear();
                indepSet_builder.ceSet = std::make_pair(-1, -1);
                indepSet_builder.cksr = -1;
                
                // build independent set -----------------
                Instance seed = InstArray[inst_id];

                    // add instance to independent set
                        // control set 可能和多线程中的处理有关系
                        // 找到 site xy -> 相同纵坐标位置内的一些inst，这些也要标记
                        // 更新 control set
                indepSet_builder.dep[inst_id] = 1;
                for (int i : seed.conn)
                {
                    indepSet_builder.dep[i] = 1; // 连坐
                }
                // ... cs ...
                indepSet_builder.set.push_back(inst_id);
                indepSet_builder.type = seed.type;

                int init_x = std::get<0>(seed.Location);
                int init_y = std::get<1>(seed.Location);

                // 用spiral accessor的方式把周围的东西遍历一遍
                std::vector<std::pair<int, int>> reachList;
                // ... fill ..., 注意不要超出边界
                for (auto xy : reachList)
                {
                    // 这里先不向z方向延展
                    // 这里做的是CLB ISM
                    int getInstID;
                    Instance inst = InstArray[getInstID];
                    if (indepSet_builder.dep[getInstID]) continue;
                    if (inst.type != indepSet_builder.type) continue;
                    // not subject to cs constraint : fes
                    // compatibility

                    // add instance <
                    indepSet_builder.dep[getInstID] = 1;
                    for (int i : inst.conn)
                    {
                        indepSet_builder.dep[i] = 1; // 连坐
                    }
                    // ... cs ...
                    indepSet_builder.set.push_back(getInstID);
                    indepSet_builder.type = inst.type;

                    if (indepSet_builder.set.size() >= 50) break;
                }
                sets.emplace_back(indepSet_builder.set);
            }  
        }
        
        // now we have many ISM. openMP -> parallel
        std::vector<std::vector<int>> Mbboxes{};
        std::vector<int> Moffset{};
        std::vector<int> MnetIds{};
        std::vector<int> Mranges{};

        std::vector<int> Msolutions{};
        std::vector<int> MsiteIds{};

        // set之间做并行
        for (int i = 0; i < sets.size(); ++i)
        {
            const auto &set = sets[i];

            // computeNetBBoxes(set, mem);
            for (int j = 0; j < set.size(); ++j)
            {
                int inst_id = set[j];
                Instance inst = InstArray[inst_id];
                auto loc = inst.Location;
                for (int pinID : inst.pin_ids)
                {
                    const auto &pin = PinArray[pinID];
                    const auto &net = NetArray[pin.net_id];
                    int a;
                    if (net.pins_id.size() > a) continue;

                    Mbboxes.emplace_back(net.bbox);
                    auto &bbox = Mbboxes.back();
                    Moffset.emplace_back(pin.offset);
                    MnetIds.emplace_back(pin.net_id);

                    bbox[0] = std::min(bbox[0], std::get<0>(loc));
                    // ... 
                }
                Mranges.push_back(Mbboxes.size());
            }

            // Compute Cost Matrix(set, mem)
            // Mtx, 他使用 Vector2D 来封装，这是用一维 Vector 包装的有两个索引的二维数组, FlowIntType = int64
            std::vector<std::vector<long long>> costMtx; // {规模就是set.size()^2}
            for (int ii = 0; ii < set.size(); ++ii)
            {
                const auto &mates = InstArray[set[i]].mate;
                for (int jj = 0; jj < set.size(); ++jj)
                {
                    double cost = 0;
                    // the moving cost of moving the i-th instance to the position of the j-th instance
                    // if not allowed, costMtx(i, j) = MAXN
                    for (int k = Mranges[i]; k < Mranges[i + 1]; ++k)
                    {
                        // get the location of this bbox, this bbox is belong to i's pin
                        // 如果新 pin 脚的 location 在其 net 的 bbox 外面就要算到 cost 里去，增量就是超出边界的距离
                        // 没有计算减量
                        // 有 mateCredit 奖励
                    }
                    costMtx[ii][jj] = cost;
                }
            }
            
            // Compute Matching(mem)
            // 问题的建模是这样的:
            // 左边和右边都放 n 个点，n 是这个支持集里面的 instance 数量
            // 仅仅用到了 costMtx
            // 如果边上有流，那就得到匹配. Msolutions[p.first] = p.second - solution里存的也是 siteID


            // Realize Matching(set, mem)
            for (int j = 0; j < set.size(); ++j)
            {
                MsiteIds[j] = InstArray[set[j]].site_id;
            }
            for (int j = 0; j < set.size(); ++j)
            {
                auto &inst = InstArray[set[j]];
                inst.site_id = MsiteIds[Msolutions[j]];
                ++inst.numMov;
                // update: inst_id in sitemap, control set, and net bounding boxes
            }
        }    
    }

    // z getter
    // write solution back to the state vector (pos)

}

// Mates
// We want LUT/FF pairs that have LUT.output->FF.input nets be mates, 也是互相存inst_id
// ism_detailed_placer.hpp, line 1030

// ISM Memory
// std::vector<Box<RealType>>  bboxes;       // Bounding boxes of nets that are incident to the instances in the set
// std::vector<Box<IndexType>> slr_bboxes;   // Bounding boxes_slr of nets that are incident to the instances in the set
// std::vector<XY<RealType>>   offset;       // offset[i] is the pin offset for the insatnce in net bounding box bboxArray[i]
// IndexVector                 netIds;       // netIds[i] is the ID of the net corresponding to bboxArray[i]
// IndexVector
//         ranges;   // [ranges[i], ranges[i+1]) contains the net bounding box related to the i-th instance in the set
// Vector2D<FlowIntType>                        costMtx;   // Cost matrix

// std::vector<bool>                            canConsider;   // If the ith element can't be swapped with ANY
// // For solving ISM
// // Since lemon::ListDigraph is not constructible, we need to wrap it with std::unique_ptr
// std::unique_ptr<lemon::ListDigraph>          graphPtr;
// std::vector<lemon::ListDigraph::Node>        lNodes;
// std::vector<lemon::ListDigraph::Node>        rNodes;
// std::vector<lemon::ListDigraph::Arc>         lArcs;
// std::vector<lemon::ListDigraph::Arc>         rArcs;
// std::vector<lemon::ListDigraph::Arc>         mArcs;
// std::vector<std::pair<IndexType, IndexType>> mArcPairs;
// IndexVector sol;   // Matching solution, the i-th instance is moved to sol[i]-th instance's location

// // General purpose buffers
// IndexVector idxVec;