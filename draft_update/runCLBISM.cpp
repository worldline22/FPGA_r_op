#include <vector>
#include <tuple>

#define MAXN 1e9

// the basic structure
struct Instance
{
    std::tuple<int, int, int> Location;
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
};
std::vector<Instance> InstArray;
std::vector<Net> NetArray;
std::vector<Pin> PinArray;



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
    int HPWL_res = 0;
    for (const auto &net : NetArray)
    {
        if (net.pins_id.size() <= 16)
        {
            HPWL_res += (net.bbox[2] - net.bbox[0]) + (net.bbox[3] - net.bbox[1]);
        }
    }
    // main iteration !
    while (true)
    {
        // the check of stop condition
        int HPWL_res = 0;
    for (const auto &net : NetArray)
    {
        if (net.pins_id.size() <= 16)
        {
            HPWL_res += (net.bbox[2] - net.bbox[0]) + (net.bbox[3] - net.bbox[1]);
        }
    }
    
    }
}