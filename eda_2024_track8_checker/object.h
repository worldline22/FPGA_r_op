
#pragma once
#include <tuple>  // 包含对 std::tuple 的支持
#include <string> // 包含对 std::string 的支持
#include <list>   // 包含对 std::list 的支持
#include <map>    // 包含对 std::map 的支持
#include <vector> // 包含对 std::vector 的支持
#include <set>    // 包含对 std::set 的支持

// PLB slots
#define MAX_LUT_CAPACITY 8
#define MAX_DFF_CAPACITY 16
#define MAX_CARRY4_CAPACITY 2
#define MAX_F7_CAPACITY 4
#define MAX_F8_CAPACITY 2
#define MAX_DRAM_CAPACITY 2
// # other slots
#define MAX_RAM_CAPACITY 1
#define MAX_DSP_CAPACITY 1
#define MAX_IO_CAPACITY 2
#define MAX_IPPIN_CAPACITY 256
#define MAX_GCLK_CAPACITY 28

// 
#define MAX_TILE_CE_PER_PLB_BANK 2
#define MAX_TILE_RESET_PER_PLB_BANK 1
#define MAX_TILE_CLOCK_PER_PLB_BANK 1

#define MAX_REGION_CLOCK_COUNT 28

#define MAX_TILE_PIN_INPUT_COUNT 112.0   // 48 LUT input pins + 16 SEQ input pins (D) + 48 SEQ ctrl pins (CE CLK SR)
#define MAX_TILE_PIN_OUTPUT_COUNT 32.0   // 16 LUT output pins + 16 SEQ output pins 

enum PinProp {
    PIN_PROP_NONE,
    PIN_PROP_CE,
    PIN_PROP_RESET,
    PIN_PROP_CLOCK
};

class Slot {
    private:
        // normally each slot is holding 1 instance
        // the exception is the LUT slot can hold up to 2 LUTs
        // with shared inputs 
        std::list<int> optimizedInstArr;  // container of the optimized design instances
        std::list<int> baselineInstArr;   // container of the input design instances
    public:
        // Constructor
        Slot() {}
        // Destructor
        ~Slot() {}

        // Getter and setter for type
        void clearInstances() {optimizedInstArr.clear(); baselineInstArr.clear();}
        void clearBaselineInstances() {baselineInstArr.clear();}
        void clearOptimizedInstances() {optimizedInstArr.clear();}

        void addOptimizedInstance(int instID) { optimizedInstArr.push_back(instID); }
        std::list<int> getOptimizedInstances() const { return optimizedInstArr; }

        void addBaselineInstance(int instID) { baselineInstArr.push_back(instID); }
        std::list<int> getBaselineInstances() const { return baselineInstArr; }
};

typedef std::vector<Slot*> slotArr;

class Tile {
    private:
        int col;
        int row;
        std::set<std::string> tileTypes;

        // container to record instances belone to this tile
        std::map<std::string, slotArr> instanceMap;

    public:
        // Constructor
        Tile(int c, int r) : col(c), row(r) {}

        // Destructor
        ~Tile();
        
        // Getter and setter
        int getCol() const { return col; }
        void setCol(int value) { col = value; }
        int getRow() const { return row; }
        void setRow(int value) { row = value; }
        
        // Getter and setter for tileTypes
        std::set<std::string> getTileTypes() const { return tileTypes; }       
        void addType(const std::string& tileType) { tileTypes.insert(tileType); }
        unsigned int getNumTileTypes() const { return tileTypes.size(); }
        
        std::string getLocStr() { return "X" + std::to_string(col) + "Y" + std::to_string(row); }

        bool initTile(const std::string& tileType);  // allocate slots
        bool matchType(const std::string& modelType); // LUT/SEQ to PLB

        bool isEmpty(bool isBaseline);        
        bool addInstance(int instID, int offset, std::string modelType, const bool isBaseline);          
        void clearInstances();
        void clearBaselineInstances();
        void clearOptimizedInstances();
        std::map<std::string, slotArr>::iterator getInstanceMapBegin() { return instanceMap.begin(); }
        std::map<std::string, slotArr>::iterator getInstanceMapEnd() { return instanceMap.end(); }
        slotArr* getInstanceByType (std::string type);
        
        bool getControlSet(
            const bool isBaseline,
            const int bank,
            std::set<int> &clkNets,
            std::set<int> &ceNets,
            std::set<int> &srNets);
        
        std::set<int> getConnectedLutSeqInput(bool isBaseline);
        std::set<int> getConnectedLutSeqOutput(bool isBaseline);

        // report util
        void reportTile();
};

class ClockRegion {
    private:
        std::string regionName;
        int xLeft;
        int xRight;  
        int yTop;
        int yBottom;
        std::set<int> clockNets;   // <netID>
    public:
        // Constructor
        ClockRegion() : regionName("undefined"), xLeft(0), xRight(0), yTop(0), yBottom(0) {
            // Add your constructor code here
        }

        // Destructor
        ~ClockRegion() {
            // Add your destructor code here
        }

        // Getter and setter for regionName
        std::string getRegionName() const {
            return regionName;
        }
        void setRegionName(const std::string& name) {
            regionName = name;
        }

        void setBoundingBox(int left, int right, int bottom, int top) {
            xLeft = left;
            xRight = right;
            yTop = top;
            yBottom = bottom;
        }

        // Getter for xLeft, xRight, yTop, yBottom
        int getXLeft() const { return xLeft; }
        int getXRight() const { return xRight; }
        int getYTop() const { return yTop; }
        int getYBottom() const { return yBottom; } 

        void addClockNet(int netID) { clockNets.insert(netID); }
        int getNumClockNets() const { return clockNets.size(); }
        void clearClockNets() { clockNets.clear(); }

        // report util
        std::string getLocStr() {return "[" + std::to_string(xLeft) + "," + std::to_string(yBottom) + "][" + std::to_string(xRight) + "," + std::to_string(yTop) + "]"; }
        void reportClockRegion();
};

class Lib {
    std::string name;
    std::vector<std::pair<std::string, PinProp> > inputs;
    std::vector<std::pair<std::string, PinProp> > outputs;

public:
    Lib(std::string libname) : name(libname)  {} // 默认构造函数
    ~Lib() {} // 析构函数

    // Getter and setter for name
    std::string getName() const { return name; }
   
    // Getter and setter for inputs
    int getNumInputs() const { return inputs.size(); }
    void setNumInputs(const int numIn) { inputs.resize(numIn); }
    std::vector<std::pair<std::string, PinProp> > getInputs() const { return inputs;}
    void setInput(int idx, std::string input, PinProp prop) { inputs[idx].first = input; inputs[idx].second = prop; }
    
    // Getter and setter for outputs
    int getNumOutputs() const { return outputs.size(); }
    void setNumOutputs(const int numOut) { outputs.resize(numOut); }
    std::vector<std::pair<std::string, PinProp> > getOutputs() const { return outputs; }
    void setOutput(int idx, std::string output, PinProp prop) { outputs[idx].first = output; outputs[idx].second = prop; }

    PinProp getInputProp(int idx) const { return inputs[idx].second; }
    PinProp getOutputProp(int idx) const { return outputs[idx].second; }
};

class Instance;

class Pin {
    int netID;
    PinProp prop;
    bool timingCritical;
    Instance* instanceOwner;

public: 
    Pin() : netID(-1), prop(PIN_PROP_NONE), timingCritical(false), instanceOwner(nullptr) {} // 默认构造函数
    ~Pin() {} // 析构函数

    // Getter and setter for netID
    int getNetID() const { return netID; }  // -1 means unconnected
    void setNetID(int value) { netID = value; }

    // Getter and setter for prop
    PinProp getProp() const { return prop; }
    void setProp(PinProp value) { prop = value; }

    // Getter and setter for isTimingCritical
    bool getTimingCritical() const { return timingCritical; }
    void setTimingCritical(bool value) { timingCritical = value; }

    // Getter and setter for instanceOwner
    Instance* getInstanceOwner() const { return instanceOwner; }
    void setInstanceOwner(Instance* inst) { instanceOwner = inst; }
};

class Instance {
    bool fixed; // 声明 fixed 成员变量
    Lib* cellLib; // 声明 cellLib 成员变量
    std::string instanceName; // 声明 instanceName 成员变量
    std::string modelName; // 声明 modelName 成员变量
    std::tuple<int, int, int> baseLocation; // location before optimization
    std::tuple<int, int, int> location; // location after optimization
    std::vector<Pin*> inpins;  
    std::vector<Pin*> outpins;

public:
    Instance(); 
    ~Instance();

    // Getter and setter
    std::tuple<int, int, int> getBaseLocation() const { return baseLocation; }
    void setBaseLocation(const std::tuple<int, int, int>& loc) { baseLocation = loc; }

    std::tuple<int, int, int> getLocation() const { return location; }
    void setLocation(const std::tuple<int, int, int>& loc) { location = loc; }

    bool isFixed() const { return fixed; }
    void setFixed(bool value) { fixed = value;}

    std::string getInstanceName() const { return instanceName; }
    void setInstanceName(const std::string& name) { instanceName = name;}

    std::string getModelName() const { return modelName; }
    void setModelName(const std::string& name) { modelName = name; }

    Lib* getCellLib() const { return cellLib; }
    void setCellLib(Lib* lib);

    bool isPlaced(); 
    bool isMoved();

    void createInpins();
    int getNumInpins() const { return inpins.size(); }
    std::vector<Pin*> getInpins() const { return inpins; }
    Pin* getInpin(int idx) const { return inpins[idx]; }
    //void connectInpin(int netID, int idx) { inpins[idx].setNetID(netID); }

    void createOutpins();
    int getNumOutpins() const { return outpins.size(); }
    std::vector<Pin*> getOutpins() const { return outpins; }
    Pin* getOutpin(int idx) const { return outpins[idx]; }
    //void connectOutpin(int netID, int idx) { outpins[idx].setNetID(netID); }

};

class Net {
    int id; // 声明 id 成员变量
    bool clock; // 声明 clock 成员变量    
    Pin* inpin;
    std::list<Pin*> outputPins;

public:
    Net(int netID) : id(netID), clock(false), inpin(nullptr) {} // 默认构造函数
    ~Net() {} // 析构函数

    // Getter and setter for id
    int getId() const { return id; }

    // Getter and setter for clock
    bool isClock() const { return clock; }
    void setClock(bool value) { clock = value; }

    // as named, is this net fanin and all fanouts are in the same tile
    bool isIntraTileNet(bool isBaseline); 
    
    // Getter and setter for inpin
    Pin* getInpin() const { return inpin; }
    void setInpin(Pin* pin) { inpin = pin; }

    // Getter and setter for outputPins
    std::list<Pin*> getOutputPins() const { return outputPins; }
    void addOutputPin(Pin* pin) { outputPins.push_back(pin); }

    int getNumPins();

    bool addConnection(std::string conn);

    int getCritWireLength(bool isBaseline);    
    void getMergedNonCritPinLocs(bool isBaseline, std::vector<int>& xCoords, std::vector<int>& yCoords);  
    int getNonCritWireLength(bool isBaseline);       

    // report util   
    bool reportNet();
};