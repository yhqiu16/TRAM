
#include "mapper/mapper.h"


Mapper::Mapper(ADG* adg): _adg(adg) {
    initializeAdg();
}

Mapper::Mapper(ADG* adg, DFG* dfg): _adg(adg), _dfg(dfg) {
    initializeAdg();
    initializeDfg();
    _mapping = new Mapping(adg, dfg);
    // initializeCandidates();
    _isDfgModified = false;
    sortDfgNodeInPlaceOrder();
}

Mapper::~Mapper(){
    if(_mapping != nullptr){
        delete _mapping;
    }
    if(_dfgModified != nullptr){
        delete _dfgModified;
    }
}

// set DFG and initialize DFG
// modify: if the DFG is a modified one
void Mapper::setDFG(DFG* dfg, bool modify){ 
    _dfg = dfg; 
    initializeDfg();
    if(_mapping != nullptr){
        delete _mapping;
    }
    _mapping = new Mapping(_adg, dfg);
    // initializeCandidates();
    if(modify){
        setDfgModified(dfg);
    } else{
        _isDfgModified = false;
    }
    sortDfgNodeInPlaceOrder();
}


// set modified DFG and delete the old one
void Mapper::setDfgModified(DFG* dfg){
    if(_dfgModified != nullptr){
        delete _dfgModified;
    }
    _dfgModified = dfg;
    _isDfgModified = true;
}

// // set ADG and initialize ADG
// void Mapper::setADG(ADG* adg){ 
//     _adg = adg; 
//     initializeAdg();
// }


// initialize mapping status of ADG
void Mapper::initializeAdg(){
    // std::cout << "Initialize ADG\n";
    calAdgNodeDist();
}


// initialize mapping status of DFG
void Mapper::initializeDfg(){
    // topoSortDfgNodes();
    _dfg->topoSortNodes();
}


// // initialize candidates of DFG nodes
// void Mapper::initializeCandidates(){
//     Candidate cdt(_mapping, 50);
//     cdt.findCandidates();
//     // candidates = cdt.candidates(); // RETURN &
//     candidatesCnt = cdt.cnt();
// }


// // sort dfg nodes in reversed topological order
// // depth-first search
// void Mapper::dfs(DFGNode* node, std::map<int, bool>& visited){
//     int nodeId = node->id();
//     if(visited.count(nodeId) && visited[nodeId]){
//         return; // already visited
//     }
//     visited[nodeId] = true;
//     for(auto& in : node->inputs()){
//         int inNodeId = in.second.first;
//         if(inNodeId == _dfg->id()){ // node connected to DFG input port
//             continue;
//         }
//         dfs(_dfg->node(inNodeId), visited); // visit input node
//     }
//     dfgNodeTopo.push_back(_dfg->node(nodeId));
// }

// // sort dfg nodes in reversed topological order
// void Mapper::topoSortDfgNodes(){
//     std::map<int, bool> visited; // node visited status
//     for(auto& in : _dfg->outputs()){
//         int inNodeId = in.second.first;
//         if(inNodeId == _dfg->id()){ // node connected to DFG input port
//             continue;
//         }
//         dfs(_dfg->node(inNodeId), visited); // visit input node
//     }
// }


// calculate the shortest path among ADG nodes
void Mapper::calAdgNodeDist(){
    // map ADG node id to continuous index starting from 0
    std::map<int, int> _adgNodeId2Idx;
    // distances among ADG nodes
    std::vector<std::vector<int>> _adgNodeDist; // [node-idx][node-idx]
    int i = 0;
    // if the ADG node with the index is GIB
    std::map<int, bool> adgNodeIdx2GIB;
    for(auto& node : _adg->nodes()){
        adgNodeIdx2GIB[i] = (node.second->type() == "GIB");
        _adgNodeId2Idx[node.first] = i++;
    }
    int n = i; // total node number
    int inf = 0x7fffffff;
    _adgNodeDist.assign(n, std::vector<int>(n, inf));
    for(auto& node : _adg->nodes()){
        int idx = _adgNodeId2Idx[node.first];
        _adgNodeDist[idx][idx] = 0;
        for(auto& src : node.second->inputs()){
            int srcId = src.second.first;
            if(srcId == _adg->id()){
                continue; // connected to ADG input port
            }
            int srcPort = src.second.second;
            ADGNode* srcNode = _adg->node(srcId);
            int dist = 1;
            if(srcNode->type() == "GIB" && node.second->type() == "GIB"){
                if(dynamic_cast<GIBNode*>(srcNode)->outReged(srcPort)){ // output port reged
                    dist = 2;
                }
            }
            int srcIdx = _adgNodeId2Idx[srcId];
            _adgNodeDist[srcIdx][idx] = dist;
        }
    }
    // Floyd algorithm
    for (int k = 0; k < n; ++k) {
        if(adgNodeIdx2GIB[k]){
            for (int i = 0; i < n; ++i) {
                for (int j = 0; j < n; ++j) {
                    if (_adgNodeDist[i][k] < inf && _adgNodeDist[k][j] < inf &&
                        _adgNodeDist[i][j] > _adgNodeDist[i][k] + _adgNodeDist[k][j]) {
                        _adgNodeDist[i][j] = _adgNodeDist[i][k] + _adgNodeDist[k][j];
                    }
                }
            }
        }        
    }

    // shortest distance between two ADG nodes (GPE/IOB nodes)
    for(auto& inode : _adg->nodes()){
        if(inode.second->type() == "GIB" || inode.second->type() == "OB"){
            continue;
        }
        int i = _adgNodeId2Idx[inode.first];
        for(auto& jnode : _adg->nodes()){
            if(jnode.second->type() == "GIB" || jnode.second->type() == "IB" ||
              (inode.second->type() == "IB" && jnode.second->type() == "OB")){
                continue;
            }
            int j = _adgNodeId2Idx[jnode.first];
            _adgNode2NodeDist[std::make_pair(inode.first, jnode.first)] = _adgNodeDist[i][j];
            // std::cout << inode.first << "," << jnode.first << ": " << _adgNodeDist[i][j] << "  ";
        }
        // std::cout << std::endl;
    }

    // // shortest distance between ADG node (GPE node) and the ADG IO
    // for(auto& inode : _adg->nodes()){
    //     if(inode.second->type() != "GPE"){
    //         continue;
    //     }
    //     int i = _adgNodeId2Idx[inode.first];
    //     int minDist2IB = inf;
    //     int minDist2OB = inf;
    //     for(auto& jnode : _adg->nodes()){            
    //         auto jtype = jnode.second->type();
    //         int j = _adgNodeId2Idx[jnode.first];
    //         if(jtype == "IB"){                
    //             minDist2IB = std::min(minDist2IB, _adgNodeDist[j][i]);
    //         }else if(jtype == "OB"){
    //             minDist2OB = std::min(minDist2OB, _adgNodeDist[i][j]);
    //         }                       
    //     }
    //     _adgNode2IODist[inode.first] = std::make_pair(minDist2IB, minDist2OB);
    //     // std::cout << inode.first << ": " << minDist2IB << "," << minDist2OB << std::endl;
    // }
}


// get the shortest distance between two ADG nodes
int Mapper::getAdgNodeDist(int srcId, int dstId){
    // return _adgNodeDist[_adgNodeId2Idx[srcId]][_adgNodeId2Idx[dstId]];
    return _adgNode2NodeDist[std::make_pair(srcId, dstId)];
}

// // get the shortest distance between ADG node and ADG input
// int Mapper::getAdgNode2InputDist(int id){
//     return _adgNode2IODist[id].first;
// }

// // get the shortest distance between ADG node and ADG input
// int Mapper::getAdgNode2OutputDist(int id){
//     return _adgNode2IODist[id].second;
// }


// calculate the number of the candidates for one DFG node
int Mapper::calCandidatesCnt(DFGNode* dfgNode, int maxCandidates){
    int candidatesCnt = 0;
    for(auto& elem : _adg->nodes()){
        auto adgNode = elem.second;
        //select GPE node
        if(adgNode->type() != "GPE"){  
            continue;
        }
        GPENode* gpeNode = dynamic_cast<GPENode*>(adgNode);
        // check if the DFG node operationis supported
        if(gpeNode->opCapable(dfgNode->operation())){
            candidatesCnt++;
        }
    }
    return std::min(candidatesCnt, maxCandidates);
}

// sort the DFG node IDs in placing order
void Mapper::sortDfgNodeInPlaceOrder(){
    std::map<int, int> candidatesCnt; // <dfgnode-id, count>
    dfgNodeIdPlaceOrder.clear();
    // topological order
    for(auto node : _dfg->topoNodes()){ 
        dfgNodeIdPlaceOrder.push_back(node->id());
        // std::cout << node->id() << ", ";
        int cnt = calCandidatesCnt(node, 50);
        candidatesCnt[node->id()] = cnt;
    }
    // std::cout << std::endl;
    // sort DFG nodes according to their candidate numbers
    // std::random_shuffle(dfgNodeIds.begin(), dfgNodeIds.end()); // randomly sort will cause long routing paths
    std::stable_sort(dfgNodeIdPlaceOrder.begin(), dfgNodeIdPlaceOrder.end(), [&](int a, int b){
        return candidatesCnt[a] <  candidatesCnt[b];
    });
}


// ===== timestamp functions >>>>>>>>>
void Mapper::setStartTime(){
    _start = std::chrono::steady_clock::now();
}


void Mapper::setTimeOut(double timeout){
    _timeout = timeout;
}


//get the running time in millisecond
double Mapper::runningTimeMS(){
    auto end = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(end-_start).count();
}



// ==== map functions below >>>>>>>>
// check if the DFG can be mapped to the ADG according to the resources
bool Mapper::preMapCheck(ADG* adg, DFG* dfg){
    // first, check the I/O port number
    if(adg->numInputs() < dfg->numInputs() || adg->numOutputs() < dfg->numOutputs()){
        std::cout << "This DFG has too many I/O port!\n";
        return false;
    }
    // second, check the total node number
    if(adg->numGpeNodes() < dfg->nodes().size()){
        std::cout << "This DFG has too many nodes!\n";
        return false;
    }
    // third, check if there are enough ADG nodes that can map the DFG nodes
    // supported operation count of ADG
    std::map<std::string, int> adgOpCnt; 
    for(auto& elem : adg->nodes()){       
        if(elem.second->type() == "GPE"){
            auto node = dynamic_cast<GPENode*>(elem.second);
            for(auto& op : node->operations()){
                if(adgOpCnt.count(op)){
                    adgOpCnt[op] += 1;
                } else {
                    adgOpCnt[op] = 1;
                }                 
            }
        }
    }
    // operation count of DFG
    std::map<std::string, int> dfgOpCnt; 
    for(auto& elem : dfg->nodes()){
        auto op = elem.second->operation();
        assert(!op.empty());
        if(dfgOpCnt.count(op)){
            dfgOpCnt[op] += 1;
        } else {
            dfgOpCnt[op] = 1;
        }
    }
    for(auto& elem : dfgOpCnt){
        if(adgOpCnt[elem.first] < elem.second){ 
            std::cout << "No enough ADG nodes to support " << elem.first << std::endl;
            return false; // there should be enough ADG nodes that support this operation
        }
    }
    return true;
}

// // map the DFG to the ADG
// bool Mapper::mapping(){

// }


// mapper with running time
bool Mapper::mapperTimed(){
    setStartTime();
    // check if the DFG can be mapped to the ADG according to the resources
    if(!preMapCheck(getADG(), getDFG())){
        return false;
    }
    std::cout << "Pre-map checking passed!\n";
    bool succeed = mapper();
    std::cout << "Running time(s): " << runningTimeMS()/1000 << std::endl;
    return succeed;
}


// execute mapping, timing sceduling, visualizing, config getting
// dumpConfig : dump configuration file
// dumpMappedViz : dump mapped visual graph
// resultDir: mapped result directory
bool Mapper::execute(bool dumpConfig, bool dumpMappedViz, std::string resultDir){
    std::cout << "Start mapping >>>>>>\n";
    bool res = mapperTimed();
    if(res){
        std::string dir;
        if(!resultDir.empty()){
            dir = resultDir;
        }else{
            dir = "results"; // default directory
        }
        if(dumpMappedViz){
            Graphviz viz(_mapping, dir);
            viz.drawDFG();
            viz.drawADG();
            viz.dumpDFGIO(); 
        }
        if(dumpConfig){
            Configuration cfg(_mapping);
            std::ofstream ofs(dir + "/config.bit");
            cfg.dumpCfgData(ofs);
        }       
        std::cout << "Succeed to map DFG to ADG!<<<<<<\n" << std::endl;
    } else{
        std::cout << "Fail to map DFG to ADG!<<<<<<\n" << std::endl;
    }
    return res;
}