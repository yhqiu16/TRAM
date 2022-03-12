
#include "mapper/mapper_sa.h"


MapperSA::MapperSA(ADG* adg, int timeout, int maxIter, bool objOpt) : Mapper(adg){
    setTimeOut(timeout);
    setMaxIters(maxIter);
    setObjOpt(objOpt);
}

// MapperSA::MapperSA(ADG* adg, DFG* dfg) : Mapper(adg, dfg){}

MapperSA::MapperSA(ADG* adg, DFG* dfg, int timeout, int maxIter, bool objOpt) : Mapper(adg, dfg){
    setTimeOut(timeout);
    setMaxIters(maxIter);
    setObjOpt(objOpt);
}

MapperSA::~MapperSA(){}

// map the DFG to the ADG, mapper API
bool MapperSA::mapper(){
    bool succeed;
    if(_objOpt){ // objective optimization
        succeed = pnrSyncOpt();
    }else{
        succeed = pnrSync(_maxIters, MAX_TEMP, true);
    }
    return succeed;
}


// PnR, Data Synchronization, and objective optimization
bool MapperSA::pnrSyncOpt(){
    int temp = MAX_TEMP; // temperature
    int maxItersMapSched = 500; // pnrSync iteration number
    int maxItersNoImprv = 50;  // if not improved for maxItersNoImprv, end
    int restartIters = 20;     // if not improved for restartIters, restart from the cached status
    int lastImprvIter = 0;
    int lastRestartIter = 0;
    int newObj;
    int oldObj = 0x7fffffff;
    int minObj = 0x7fffffff;
    bool succeed = false;
    Mapping* bestMapping = new Mapping(getADG(), getDFG());
    Mapping* lastAcceptMapping = new Mapping(getADG(), getDFG());
    for(int iter = 0; iter < _maxIters; iter++){
        if(runningTimeMS() > getTimeOut()){
            break;
        }
        // PnR and Data Synchronization 
        bool res = pnrSync(maxItersMapSched, temp, (!succeed));
        if(!res){ // fail to map
            spdlog::debug("PnR and Data Synchronization failed!");
            continue;
        }
        succeed = true;
        spdlog::info("PnR and Data Synchronization succeed, start optimization");
        // Objective function
        newObj = objFunc(_mapping);
        spdlog::debug("Object: {}", newObj);
        int difObj = newObj - oldObj;
        if(metropolis(difObj, temp)){ // accept new solution according to the Metropolis rule
            if(newObj < minObj){ // get better result
                minObj = newObj;
                *bestMapping = *_mapping; // cache better mapping status, ##### DEFAULT "=" IS OK #####
                lastImprvIter = iter; 
                lastRestartIter = iter; 
                temp = annealFunc(temp); //  annealling
                spdlog::warn("###### Better object: {} ######", newObj);
            }
            *lastAcceptMapping = *_mapping; // can keep trying based on current status          
            oldObj = newObj;
        }else{
            *_mapping = *lastAcceptMapping; // restart from the cached status 
        }
        if(iter - lastImprvIter > maxItersNoImprv){ // if not improved for long time, STOP            
            break;
        }
        if(iter - lastRestartIter > restartIters){ // if not improved for some time, restart from the cached status 
            *_mapping = *bestMapping;
            lastRestartIter = iter;    
        } 
    }
    *_mapping = *bestMapping;
    delete bestMapping; 
    delete lastAcceptMapping;
    if(succeed){
        spdlog::warn("######## Best object: {} ########", minObj);
        std::cout << "Best max latency: " << _mapping->maxLat() << std::endl;
    }    
    return succeed;
}


// PnR and Data Synchronization
bool MapperSA::pnrSync(int maxIters, int temp, bool modifyDfg){
    int initTemp = temp;
    ADG* adg = _mapping->getADG();
    DFG* dfg = _mapping->getDFG();
    Mapping* curMapping = new Mapping(adg, dfg);
    Mapping* lastAcceptMapping = new Mapping(adg, dfg);
    int numNodes = dfg->nodes().size();
    int maxItersNoImprv = 20 + numNodes/5; // if not improved for maxItersNoImprv, end
    // int restartIters = 20;     // if not improved for restartIters, restart from the cached status
    int lastImprvIter = 0;
    // int lastRestartIter = 0;
    bool succeed = false;
    bool update = false;
    int newVio;
    int oldVio = 0x7fffffff;
    int minVio = 0x7fffffff;
    for(int iter = 0; iter < maxIters; iter++){
        if(runningTimeMS() > getTimeOut()){
            break;
        }
        // if(iter & 0xf == 0){
        //     std::cout << ".";
        // }
        // PnR without latency scheduling of DFG nodes
        int status = pnr(curMapping, temp);
        if(status == -1){ // fail to map
            spdlog::debug("PnR failed once!");
            continue;
        }
        spdlog::info("PnR succeed, start data synchronization");
        // Data synchronization : schedule the latency of DFG nodes
        curMapping->latencySchedule();
        spdlog::info("Complete data synchronization, check latency violation");
        newVio = curMapping->totalViolation(); // latency violations
        if(newVio == 0){
            succeed = true;
            *_mapping = *curMapping; // keep better mapping status, ##### DEFAULT "=" IS OK #####
            break;
        }
        int difVio = newVio - oldVio;
        if(metropolis(difVio, temp)){ // accept new solution according to the Metropolis rule
            if(newVio < minVio){ // get better result
                minVio = newVio;
                *_mapping = *curMapping; // cache better mapping status, ##### DEFAULT "=" IS OK #####
                lastImprvIter = iter; 
                // lastRestartIter = iter; 
                temp = annealFunc(temp); //  annealling
                update = true;
                spdlog::warn("#### Smaller violation: {} ####", minVio);
            }
            *lastAcceptMapping = *curMapping; // can keep trying based on current status            
            oldVio = newVio;
        }else{
            *curMapping = *lastAcceptMapping; 
        }
        // if not improved for long time, insert pass-through nodes
        if(iter - lastImprvIter > maxItersNoImprv){ 
            if(!modifyDfg){ // cannot modify DFG, stop iteration
                break;
            }                                  
            DFG* newDfg = new DFG();
            int totalVio, maxVio;
            if(update){
                totalVio = _mapping->totalViolation();
                maxVio = _mapping->maxViolation();
                _mapping->insertPassDfgNodes(newDfg); // insert pass-through nodes into DFG
            }else{
                totalVio = lastAcceptMapping->totalViolation();
                maxVio = lastAcceptMapping->maxViolation();
                lastAcceptMapping->insertPassDfgNodes(newDfg); // insert pass-through nodes into DFG
            }            
            spdlog::warn("Min total latency violation: {}", minVio);
            spdlog::warn("Current total latency violation: {}", totalVio); 
            spdlog::warn("Current max latency violation: {}", maxVio);  
            spdlog::warn("Insert pass-through nodes into DFG");
            // newDfg->print();                
            setDFG(newDfg, true);  //  update the _dfg and initialize                                                           
            if(!preMapCheck(adg, newDfg)){
                break;
            }
            delete curMapping; 
            curMapping = new Mapping(adg, newDfg);
            *lastAcceptMapping = *curMapping;
            lastImprvIter = iter; 
            // lastRestartIter = iter; 
            temp = initTemp;
            oldVio = 0x7fffffff;
            minVio = 0x7fffffff;
            update = false;
            int numNodesNew = _mapping->getDFG()->nodes().size();
            maxItersNoImprv = 20 + numNodesNew/5 + numNodesNew - numNodes;
            spdlog::warn("DFG node number: {}", numNodesNew);
            // continue;
        }
        // if(iter - lastRestartIter > restartIters){ // if not improved for some time, restart from the cached status 
        //     *curMapping = *_mapping;
        //     lastRestartIter = iter;    
        // } 
    }
    delete curMapping; 
    delete lastAcceptMapping;
    if(succeed){
        spdlog::warn("Max latency: {}", _mapping->maxLat());
    }    
    return succeed;
}



// PnR with SA temperature(max = 100)
int MapperSA::pnr(Mapping* mapping, int temp){
    unmapSome(mapping, temp);
    return incrPnR(mapping);
}


// unmap some DFG nodes
void MapperSA::unmapSome(Mapping* mapping, int temp){
    for(auto& elem : mapping->getDFG()->nodes()){
        auto node = elem.second;
        if((rand()%MAX_TEMP < temp) && mapping->isMapped(node)){
            mapping->unmapDfgNode(node);
        }
    }
}


// incremental PnR, try to map all the left DFG nodes based on current mapping status
int MapperSA::incrPnR(Mapping* mapping){
    auto dfg = mapping->getDFG();
    // // cache DFG node IDs
    // std::vector<int> dfgNodeIds;
    // for(auto node : dfg->topoNodes()){ // topological order
    //     dfgNodeIds.push_back(node->id());
    //     // std::cout << node->id() << ", ";
    // }
    // // std::cout << std::endl;
    // // sort DFG nodes according to their candidate numbers
    // // std::random_shuffle(dfgNodeIds.begin(), dfgNodeIds.end()); // randomly sort will cause long routing paths
    // std::stable_sort(dfgNodeIds.begin(), dfgNodeIds.end(), [&](int a, int b){
    //     return candidatesCnt[a] <  candidatesCnt[b];
    // });
    // for(auto id : dfgNodeIds){ // topological order
    //     std::cout << id << "(" << candidatesCnt[id] << ")" << ", ";
    // }
    // std::cout << std::endl;
    // start mapping
    bool succeed = true;
    // Candidate cdt(mapping, 50);
    // Candidate cdt(mapping, getADG()->numGpeNodes());
    for(int id : dfgNodeIdPlaceOrder){        
        auto dfgNode = dfg->node(id);
        // std::cout << "Mapping DFG node " << dfgNode->name() << ", id: " << id << std::endl;        
        if(!mapping->isMapped(dfgNode)){
            // find candidate ADG nodes for this DFG node
            // cdt.findCandidates(dfgNode);
            // auto& nodeCandidates = cdt.candidates(dfgNode->id());
            auto nodeCandidates = findCandidates(mapping, dfgNode, 50);
            if(nodeCandidates.empty() || tryCandidates(mapping, dfgNode, nodeCandidates) == -1){
                // std::cout << "Cannot map DFG node " << dfgNode->id() << std::endl;
                spdlog::debug("Cannot map DFG node {0} : {1}", dfgNode->id(), dfgNode->name());
                // for(auto& cdt : nodeCandidates){
                //     spdlog::debug("ADG node candidate {}", cdt->id());
                // }
                // Graphviz viz(mapping, "results");
                // viz.printDFGEdgePath();
                succeed = false;
                break;
            }
        }
        // spdlog::debug("Mapping DFG node {0} : {1} to ADG node {2}", dfgNode->name(), id, mapping->mappedNode(dfgNode)->id());
    }
    return succeed? 1 : -1;    
}



// try to map one DFG node to one of its candidates
// return selected candidate index
int MapperSA::tryCandidates(Mapping* mapping, DFGNode* dfgNode, const std::vector<ADGNode*>& candidates){
    // // sort candidates according to their distances with the mapped src and dst ADG nodes of this DFG node 
    // std::vector<int> sortedIdx = sortCandidates(mapping, dfgNode, candidates);
    int idx = 0;
    for(auto& candidate : candidates){
        if(tryCandidate(mapping, dfgNode, candidate)){            
            return idx;
        }
        idx++;
    }
    return -1;
}

// find candidates for one DFG node based on current mapping status
std::vector<ADGNode*> MapperSA::findCandidates(Mapping* mapping, DFGNode* dfgNode, int maxCandidates){
    std::vector<ADGNode*> candidates;
    for(auto& elem : mapping->getADG()->nodes()){
        auto adgNode = elem.second;
        //select GPE node
        if(adgNode->type() != "GPE"){  
            continue;
        }
        GPENode* gpeNode = dynamic_cast<GPENode*>(adgNode);
        // check if the DFG node operationis supported
        if(!gpeNode->opCapable(dfgNode->operation())){
            continue;
        }
        if(!mapping->isMapped(gpeNode)){
            candidates.push_back(gpeNode);
        }
    }
    // randomly select candidates
    std::random_shuffle(candidates.begin(), candidates.end());
    int num = std::min((int)candidates.size(), maxCandidates);
    candidates.erase(candidates.begin()+num, candidates.end());
    // sort candidates according to their distances with the mapped src and dst ADG nodes of this DFG node 
    std::vector<int> sortedIdx = sortCandidates(mapping, dfgNode, candidates);
    std::vector<ADGNode*> sortedCandidates;
    for(int i = 0; i < num; i++){
        sortedCandidates.push_back(candidates[sortedIdx[i]]);
    }
    return sortedCandidates;
}


// get the shortest distance between ADG node and the available ADG input
int MapperSA::getAdgNode2InputDist(Mapping* mapping, int id){
    // shortest distance between ADG node (GPE node) and the ADG IO
    int minDist = 0x7fffffff;
    for(auto& jnode : getADG()->nodes()){            
        if(jnode.second->type() == "IB" && mapping->isIBAvail(jnode.second)){                
            minDist = std::min(minDist, getAdgNodeDist(jnode.first, id));
        }                   
    }
    return minDist;
}

// get the shortest distance between ADG node and the available ADG output
int MapperSA::getAdgNode2OutputDist(Mapping* mapping, int id){
    int minDist = 0x7fffffff;
    for(auto& jnode : getADG()->nodes()){            
        if(jnode.second->type() == "OB" && mapping->isOBAvail(jnode.second)){                
            minDist = std::min(minDist, getAdgNodeDist(id, jnode.first));
        }                   
    }
    return minDist;
}

// sort candidates according to their distances with the mapped src and dst ADG nodes of this DFG node 
// return sorted index of candidates
std::vector<int> MapperSA::sortCandidates(Mapping* mapping, DFGNode* dfgNode, const std::vector<ADGNode*>& candidates){
    // mapped ADG node IDs of the source and destination node of this DFG node
    std::vector<int> srcAdgNodeId, dstAdgNodeId; 
    int num2in = 0;  // connected to DFG input port
    int num2out = 0; // connected to DFG output port
    DFG* dfg = mapping->getDFG();
    for(auto& elem : dfgNode->inputs()){
        int inNodeId = elem.second.first;
        if(inNodeId == dfg->id()){ // connected to DFG input port
            if(mapping->isDfgInputMapped(elem.second.second)){ // the DFG input port already mapped
                auto ibId = getADG()->input(mapping->dfgInputAttr(elem.second.second).adgIOPort).begin()->first;
                srcAdgNodeId.push_back(ibId);
            }else{
                num2in++;
            }
            continue;
        }
        auto inNode = dfg->node(inNodeId);
        auto adgNode = mapping->mappedNode(inNode);
        if(adgNode){
            srcAdgNodeId.push_back(adgNode->id());
        }
    }
    for(auto& elem : dfgNode->outputs()){
        for(auto outNode : elem.second){
            if(outNode.first == dfg->id()){ // connected to DFG output port
                num2out++;
                continue;
            }
            auto adgNode = mapping->mappedNode(dfg->node(outNode.first));
            if(adgNode){
                dstAdgNodeId.push_back(adgNode->id());
            }
        }        
    }
    // sum distance between candidate and the srcAdgNode & dstAdgNode & IO
    std::vector<int> sortedIdx, sumDist; // <candidate-index, sum-distance>
    for(int i = 0; i < candidates.size(); i++){
        int sum = 0;
        int cdtId = candidates[i]->id();
        for(auto id : srcAdgNodeId){
            sum += getAdgNodeDist(id, cdtId);
        }
        for(auto id : dstAdgNodeId){
            sum += getAdgNodeDist(cdtId, id);
        }
        sum += num2in * getAdgNode2InputDist(mapping, cdtId);
        sum += num2out * getAdgNode2OutputDist(mapping, cdtId);
        sumDist.push_back(sum);
        sortedIdx.push_back(i);
    }
    std::sort(sortedIdx.begin(), sortedIdx.end(), [&sumDist](int a, int b){
        return sumDist[a] < sumDist[b];
    });
    return sortedIdx;
}


// try to map one DFG node to one candidates
bool MapperSA::tryCandidate(Mapping* mapping, DFGNode* dfgNode, ADGNode* candidate){
    DFG* dfg = mapping->getDFG();
    bool succeed = true;
    std::vector<DFGEdge*> routedEdges;
    // route the src edges whose src nodes have been mapped or connected to DFG input port
    for(auto& elem : dfgNode->inputEdges()){
        DFGEdge* edge = dfg->edge(elem.second);
        if(edge->srcId() == dfg->id()){ // connected to DFG input port
            succeed = mapping->routeDfgEdge(edge, candidate, true); // route edge between candidate and ADG IB
            if(succeed){
                routedEdges.push_back(edge); // cache the routed edge
                continue;
            }else{
                break;
            }
        }
        DFGNode* inNode = dfg->node(edge->srcId());
        ADGNode* adgNode = mapping->mappedNode(inNode);
        if(adgNode){
            succeed = mapping->routeDfgEdge(edge, adgNode, candidate); // route edge between candidate and adgNode
            if(succeed){
                routedEdges.push_back(edge); // cache the routed edge
                continue;
            }else{
                break;
            }
        }
    }
    if(succeed){
        // route the dst edge whose dst nodes have been mapped or connected to DFG output port
        for(auto& elem : dfgNode->outputEdges()){
            for(int outEdgeId : elem.second){
                DFGEdge* edge = dfg->edge(outEdgeId);
                if(edge->dstId() == dfg->id()){ // connected to DFG output port
                    succeed = mapping->routeDfgEdge(edge, candidate, false); // route edge between candidate and ADG OB
                    if(succeed){
                        routedEdges.push_back(edge); // cache the routed edge
                        continue;
                    }else{
                        break;
                    }
                }
                DFGNode* outNode = dfg->node(edge->dstId());
                ADGNode* adgNode = mapping->mappedNode(outNode);
                if(adgNode){
                    succeed = mapping->routeDfgEdge(edge, candidate, adgNode); // route edge between candidate and adgNode
                    if(succeed){
                        routedEdges.push_back(edge); // cache the routed edge
                        continue;
                    }else{
                        break;
                    }
                }
            }        
        }
    }
    if(!succeed){
        for(auto re : routedEdges){ // unroute all the routed edges
            mapping->unrouteDfgEdge(re);
        }
        return false;
    }
    // map the DFG node to this candidate
    return mapping->mapDfgNode(dfgNode, candidate);
}




// objective funtion
int MapperSA::objFunc(Mapping* mapping){
    int maxEdgeLen = 0;
    int totalEdgeLen = 0;
    mapping->getEdgeLen(totalEdgeLen, maxEdgeLen);
    int obj = mapping->maxLat() * 100 +
              maxEdgeLen * 10 + totalEdgeLen;
    return obj;
}


// SA the probablity of accepting new solution
bool MapperSA::metropolis(double diff, double temp){
    if(diff < 0){
        return true;
    }else{
        double val = static_cast<double>(rand()) / static_cast<double>(RAND_MAX);
        return val < exp(-diff/temp);
    }
}


// annealing funtion
int MapperSA::annealFunc(int temp){
    float k = 0.9;
    return int(k*temp);
}