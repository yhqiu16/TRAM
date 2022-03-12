
#include "mapper/mapping.h"


// reset mapping intermediate result and status
void Mapping::reset(){
    _dfgNodeAttr.clear();
    _dfgEdgeAttr.clear();
    _adgNodeAttr.clear();
    // _adgLinkAttr.clear();
    _totalViolation = 0;
    _numNodeMapped = 0;
}


// if this input port of this ADG node is used
bool Mapping::isAdgNodeInPortUsed(int nodeId, int portIdx){
    if(_adgNodeAttr.count(nodeId)){
        auto& status = _adgNodeAttr[nodeId].inPortUsed;
        if(status.count(portIdx)){
            return status[portIdx];
        }
    }
    return false;
}


// if this output port of this ADG node is used
bool Mapping::isAdgNodeOutPortUsed(int nodeId, int portIdx){
    if(_adgNodeAttr.count(nodeId)){
        auto& status = _adgNodeAttr[nodeId].outPortUsed;
        if(status.count(portIdx)){
            return status[portIdx];
        }
    }
    return false;
}


// if the DFG node is already mapped
bool Mapping::isMapped(DFGNode* dfgNode){
    if(_dfgNodeAttr.count(dfgNode->id())){
        return _dfgNodeAttr[dfgNode->id()].adgNode != nullptr;
    }
    return false;
    
}

// if the ADG node is already mapped
bool Mapping::isMapped(ADGNode* adgNode){
    if(_adgNodeAttr.count(adgNode->id())){
        return _adgNodeAttr[adgNode->id()].dfgNode != nullptr;
    }
    return false;
}

// if the IB node is available 
bool Mapping::isIBAvail(ADGNode* adgNode){
    assert(adgNode->type() == "IB");
    int id = adgNode->id();
    if(_adgNodeAttr.count(id)){
        auto& inPortUsed = _adgNodeAttr[id].inPortUsed;
        for(int i = 0; i < adgNode->numInputs(); i++){
            if(!inPortUsed.count(i) || !inPortUsed[i]){
                return true;
            }
        }
        return false;
    }
    return true;
}

// if the OB node is available 
bool Mapping::isOBAvail(ADGNode* adgNode){
    assert(adgNode->type() == "OB");
    int id = adgNode->id();
    if(_adgNodeAttr.count(id)){
        auto& outPortUsed = _adgNodeAttr[id].outPortUsed;
        for(int i = 0; i < adgNode->numOutputs(); i++){
            if(!outPortUsed.count(i) || !outPortUsed[i]){
                return true;
            }
        }
        return false;
    }
    return true;
}

// if the DFG input port is mapped
bool Mapping::isDfgInputMapped(int idx){
    if(_dfgInputAttr.count(idx) && !_dfgInputAttr[idx].routedEdgeIds.empty()){
        return true;
    }
    return false;
}

// if the DFG output port is mapped
bool Mapping::isDfgOutputMapped(int idx){
    if(_dfgOutputAttr.count(idx) && !_dfgOutputAttr[idx].routedEdgeIds.empty()){
        return true;
    }
    return false;
}

// occupied ADG node of this DFG node
ADGNode* Mapping::mappedNode(DFGNode* dfgNode){
    return _dfgNodeAttr[dfgNode->id()].adgNode;
}


// mapped DFG node of this ADG node
DFGNode* Mapping::mappedNode(ADGNode* adgNode){
    return _adgNodeAttr[adgNode->id()].dfgNode;
}


// map DFG node to ADG node, not assign ports
// check if there is constant operand
bool Mapping::mapDfgNode(DFGNode* dfgNode, ADGNode* adgNode){
    int dfgNodeId = dfgNode->id();
    if(_dfgNodeAttr.count(dfgNodeId)){
        if(_dfgNodeAttr[dfgNodeId].adgNode == adgNode){
            return true;
        } else if(_dfgNodeAttr[dfgNodeId].adgNode != nullptr){
            return false;
        }
    } 
    DFGNodeAttr dfgAttr;
    dfgAttr.adgNode = adgNode;
    _dfgNodeAttr[dfgNodeId] = dfgAttr;
    ADGNodeAttr adgAttr;
    adgAttr.dfgNode = dfgNode;
    // one operand is immediate and the operands are not commutative
    if(dfgNode->hasImm() && !dfgNode->commutative()){ // assign constant port
        GPENode* gpeNode = dynamic_cast<GPENode*>(adgNode);
        for(auto inPort : gpeNode->operandInputs(dfgNode->immIdx())){
            if(adgAttr.inPortUsed.count(inPort) && adgAttr.inPortUsed[inPort] == true){ // already used
                return false;
            }
            adgAttr.inPortUsed[inPort] = true; // set all the input ports connetced to this operand as used
        }
    }
    _adgNodeAttr[adgNode->id()] = adgAttr;
    _numNodeMapped++;
    return true;
}


// unmap DFG node 
void Mapping::unmapDfgNode(DFGNode* dfgNode){
    // unroute the input edges of this node
    for(auto& elem : dfgNode->inputEdges()){ // input-idx, edge-id
        int inEdgeId = elem.second; // 
        unrouteDfgEdge(_dfg->edge(inEdgeId));
    }
    // unroute the output edges of this node
    for(auto& elem : dfgNode->outputEdges()){ // input-idx, edge-id
        auto& outEdgeIds = elem.second; // 
        for(auto it = outEdgeIds.begin(); it != outEdgeIds.end(); it++){
            unrouteDfgEdge(_dfg->edge(*it));
        }
    }
    // unmap the mapped ADG Node
    ADGNode* adgNode = _dfgNodeAttr[dfgNode->id()].adgNode;
    if(adgNode){
        _adgNodeAttr.erase(adgNode->id());
    }
    _dfgNodeAttr.erase(dfgNode->id());
    _numNodeMapped--;
}


// // if the ADG link is already routed
// bool Mapping::isRouted(ADGLink* link){
//     return !_adgLinkAttr[link->id()].dfgEdges.empty();
// }

// routed DFG edge links, including inter-node and intra-node links
const std::vector<EdgeLinkAttr>& Mapping::routedEdgeLinks(DFGEdge* edge){
    assert(_dfgEdgeAttr.count(edge->id()));
    return _dfgEdgeAttr[edge->id()].edgeLinks;
}

// /* route DFG edge to ADG link
// *  one link can route multiple edges, but they should have the same srcId and srcPortIdx 
// */
// bool Mapping::routeDfgEdge(DFGEdge* edge, ADGLink* link){
//     if(_adgLinkAttr.count(link->id())){
//         for(auto& linkEdge : _adgLinkAttr[link->id()].dfgEdges){
//             if(linkEdge->srcId() != edge->srcId() || linkEdge->srcPortIdx() != edge->srcPortIdx()){
//                 return false; 
//             }
//         }
//     }
//     _adgLinkAttr[link->id()].dfgEdges.push_back(edge);
//     EdgeLinkAttr attr;
//     attr.isLink = true;
//     attr.edgeLink.adgLink = link;
//     _dfgEdgeAttr[edge->id()].edgeLinks.push_back(attr);
//     return true;
// }


// route DFG edge to passthrough ADG node
// one internal link can passthrough multiple edges, but they should have the same srcId and srcPortIdx
// isTry: just try to route, not change the routing status
bool Mapping::routeDfgEdgePass(DFGEdge* edge, ADGNode* passNode, int srcPort, int dstPort, bool isTry){
    int passNodeId = passNode->id();
    int routeSrcPort = srcPort;
    int routeDstPort = dstPort;
    bool hasSameSrcEdge = false;
    if(_adgNodeAttr.count(passNodeId)){
        auto& passNodeAttr = _adgNodeAttr[passNodeId];
        // check if conflict with current routed edges
        for(auto& edgeLink : passNodeAttr.dfgEdgePass){
            auto passEdge = edgeLink.edge;
            if((edgeLink.srcPort == srcPort || edgeLink.dstPort == dstPort) &&  // occupy the same port
               (passEdge->srcId() != edge->srcId() || passEdge->srcPortIdx() != edge->srcPortIdx())){
                return false; 
            } else if((edgeLink.srcPort == srcPort || edgeLink.dstPort == dstPort) &&  // try to route same-source edge to same internal link
                       passEdge->srcId() == edge->srcId() && passEdge->srcPortIdx() == edge->srcPortIdx()){ 
                routeSrcPort = edgeLink.srcPort; // default route link
                routeDstPort = edgeLink.dstPort;
                hasSameSrcEdge = true;
            } 
        }
        if(srcPort >= 0 && dstPort >= 0){ // manually assign srcPort and dstPort
            if(!passNode->isInOutConnected(srcPort, dstPort)){
                return false;
            } else if(isAdgNodeOutPortUsed(passNodeId, dstPort) && (!hasSameSrcEdge || (routeSrcPort != srcPort) || (routeDstPort != dstPort))){
                return false; // if dstPort used, must have same-source edge with same srcPort and dstPort
            } else if(isAdgNodeInPortUsed(passNodeId, srcPort) && (!hasSameSrcEdge || (routeSrcPort != srcPort))){
                return false; // if srcPort used, must have same-source edge with same srcPort
            }   
            routeSrcPort = srcPort;
            routeDstPort = dstPort;         
        } else if(srcPort >= 0){ // auto-assign dstPort           
            if(isAdgNodeInPortUsed(passNodeId, srcPort) && (!hasSameSrcEdge || (routeSrcPort != srcPort))){ 
                return false; // no same-source edge or have different srcPort
            }
            if(!isAdgNodeInPortUsed(passNodeId, srcPort)){                    
                bool flag = false;
                for(auto port : passNode->in2outs(srcPort)){
                    if(isAdgNodeOutPortUsed(passNodeId, port)){ // already used
                        continue;
                    }
                    // find one available port
                    routeSrcPort = srcPort;
                    routeDstPort == port;
                    flag = true;
                    break;
                }
                if(!flag){ // cannot find one available port
                    return false;
                }
            }           
            // if have same-source edge and same srcPort, select the same dstPort            
        } else if(dstPort >= 0){ // auto-assign srcPort            
            if(isAdgNodeOutPortUsed(passNodeId, dstPort) && (!hasSameSrcEdge || (routeDstPort != dstPort))){
                return false;
            }
            if(!isAdgNodeOutPortUsed(passNodeId, dstPort)){ // no same-source edge or have different dstPort
                bool flag = false;
                for(auto port : passNode->out2ins(dstPort)){
                    if(isAdgNodeInPortUsed(passNodeId, port)){ // already used
                        continue;
                    }
                    // find one available port
                    routeSrcPort == port;
                    routeDstPort = dstPort;
                    flag = true;
                    break;                    
                }
                if(!flag){ // cannot find one available port
                    return false;
                }                
            }            
            // if have same-source edge with same dstPort, select the same srcPort and dstPort          
        } else { // auto-assign srcPort and dstPort
            if(!hasSameSrcEdge){
                bool outflag = false;
                for(auto& elem : passNode->outputs()){
                    int outPort = elem.first;
                    if(isAdgNodeOutPortUsed(passNodeId, outPort)){ // already used
                        continue;
                    }
                    bool inflag = false;
                    for(auto inPort : passNode->out2ins(outPort)){
                        if(isAdgNodeInPortUsed(passNodeId, inPort)){ // already used
                            continue;
                        }
                        // find one available inport
                        routeSrcPort == inPort;
                        routeDstPort = outPort;
                        inflag = true;
                        break;                        
                    }
                    if(inflag){ // find one available port
                        outflag = true;
                        break;
                    }
                }
                if(!outflag){ // cannot find one available port
                    return false;
                }
            }
        }
    } else { // _adgNodeAttr.count(passNodeId) = 0; this passNode has not been used
        bool outflag = false;
        for(auto& elem : passNode->outputs()){
            int outPort = elem.first;
            auto inPorts = passNode->out2ins(outPort);
            if(!inPorts.empty()){ // find one available inport
                routeSrcPort == *(inPorts.begin());
                routeDstPort = outPort;
                outflag = true;
                break;
            }
        }
        if(!outflag){ // cannot find one available port
            return false;
        }
    }

    if(!isTry){
        EdgeLinkAttr edgeLinkAttr;    
        edgeLinkAttr.srcPort = routeSrcPort;
        edgeLinkAttr.dstPort = routeDstPort;
        edgeLinkAttr.adgNode = passNode;
        _dfgEdgeAttr[edge->id()].edgeLinks.push_back(edgeLinkAttr);
        DfgEdgePassAttr edgePassAttr;
        edgePassAttr.edge = edge;
        edgePassAttr.srcPort = routeSrcPort;
        edgePassAttr.dstPort = routeDstPort;
        _adgNodeAttr[passNodeId].inPortUsed[routeSrcPort] = true;
        _adgNodeAttr[passNodeId].outPortUsed[routeDstPort] = true;
        _adgNodeAttr[passNodeId].dfgEdgePass.push_back(edgePassAttr);        
    }
    return true;
}


// route DFG edge between srcNode and dstNode/OB
// find a routable path from srcNode to dstNode/OB by BFS
// dstNode = nullptr: OB
// dstPortRange: the input port index range of the dstNode
bool Mapping::routeDfgEdgeFromSrc(DFGEdge* edge, ADGNode* srcNode, ADGNode* dstNode, const std::set<int>& dstPortRange){
    struct VisitNodeInfo{
        // int inPortIdx;  // input port index of this node 
        int srcNodeId;  // src node ID
        int srcInPortIdx; // input port index of src node
        int srcOutPortIdx; // output port index of src node
    };
    // cache visited node information, <<node-id, inport-index>, VisitNodeInfo>
    std::map<std::pair<int, int>, VisitNodeInfo> visitNodes; 
    // cache visited nodes, <node, inport-index>
    std::queue<std::pair<ADGNode*, int>> nodeQue; 
    // Breadth first search for possible routing path
    // assign the index of the output port of the srcNode
    int srcNodePortIdx = edge->srcPortIdx();
    int dstNodePortIdx;
    int mappedAdgOutPort;
    std::pair<int, int> finalDstNode; // <node-id, inport-index>
    bool success = false;
    nodeQue.push(std::make_pair(srcNode, -1));
    while(!nodeQue.empty()){
        auto queHead = nodeQue.front();
        ADGNode* adgNode = queHead.first;
        int inPortIdx = queHead.second;
        nodeQue.pop();
        VisitNodeInfo info;
        std::vector<int> outPortIdxs;
        if(inPortIdx == -1){ // srcNode
            outPortIdxs.push_back(srcNodePortIdx);
        }else{ // intermediate node
            for(int outPortIdx : adgNode->in2outs(inPortIdx)){
                if(!routeDfgEdgePass(edge, adgNode, inPortIdx, outPortIdx, true)){ // try to find an internal link to route the edge
                    continue;
                }
                outPortIdxs.push_back(outPortIdx); // collect all available outPort 
            }   
        } 
        // search this layer of nodes
        for(int outPortIdx : outPortIdxs){
            for(auto& elem : adgNode->output(outPortIdx)){
                int nextNodeId = elem.first;
                int nextSrcPort = elem.second;
                auto nextId = std::make_pair(nextNodeId, nextSrcPort);
                ADGNode* nextNode = _adg->node(nextNodeId);
                auto nextNodeType = nextNode->type();                
                if(dstNode != nullptr){ // route to dstNode
                    if((dstNode->id() == nextNodeId) && dstPortRange.count(nextSrcPort)){ // get to the dstNode                    
                        success = true;
                        finalDstNode = nextId;
                    } else if((dstNode->id() == nextNodeId) || (nextNodeType == "GPE") || // not use GPE node to route
                              (nextNodeType == "OB") || // cannot route to the dstNode through IOB
                              visitNodes.count(nextId) || // the <node-id, inport-index> already visited
                              isAdgNodeInPortUsed(nextNodeId, nextSrcPort)){ // the input port is already used
                        continue;
                    }
                } else{  // route to OB
                    if(nextNodeType == "OB"){
                        for(int nextDstPortIdx : nextNode->in2outs(nextSrcPort)){
                            if(routeDfgEdgePass(edge, nextNode, nextSrcPort, nextDstPortIdx, true)){ // try to find an internal link to route the edge
                                success = true;
                                finalDstNode = nextId;
                                dstNodePortIdx = nextDstPortIdx;
                                mappedAdgOutPort = nextNode->output(nextDstPortIdx).begin()->second;
                                break;
                            }
                        }
                        if(!success) continue;
                         
                    } else if((nextNodeType == "GPE") || // not use GPE node to route
                              visitNodes.count(nextId) || // the <node-id, inport-index> already visited
                              isAdgNodeInPortUsed(nextNodeId, nextSrcPort)){ // the input port is already used
                        continue;
                    }
                }                
                nodeQue.push(std::make_pair(nextNode, nextSrcPort)); // cache in queue
                info.srcNodeId = adgNode->id(); //  current node ID
                info.srcInPortIdx = inPortIdx;  // input port index of src node
                info.srcOutPortIdx = outPortIdx;   //  output port index of current node 
                visitNodes[nextId] = info;
                if(success){
                    break;
                }  
            }
            if(success){
                break;
            }
        }
        if(success){
            break;
        }
    }
    if(!success){
        return false;
    }
    // route the found path
    auto routeNode = finalDstNode;
    auto& edgeLinks = _dfgEdgeAttr[edge->id()].edgeLinks;
    int dstPort = (dstNode)? -1 : dstNodePortIdx;
    while (true){
        int nodeId = routeNode.first;
        int srcPort = routeNode.second;
        // keep the DFG edge routing status
        EdgeLinkAttr edgeAttr;
        edgeAttr.srcPort = srcPort;
        edgeAttr.dstPort = dstPort;
        edgeAttr.adgNode = _adg->node(nodeId);        
        edgeLinks.push_back(edgeAttr);
        // keep the ADG Node routing status
        ADGNodeAttr& nodeAttr = _adgNodeAttr[nodeId];
        if(dstNode != nullptr && routeNode == finalDstNode){ // dstNode
            nodeAttr.inPortUsed[srcPort] = true;  // only change the input port status
        } else if(nodeId == srcNode->id()){ // srcNode
            nodeAttr.outPortUsed[dstPort] = true; // only change the output port status
            break; // get to the srcNode
        } else{ // intermediate routing nodes or OB
            nodeAttr.inPortUsed[srcPort] = true;
            nodeAttr.outPortUsed[dstPort] = true;
            DfgEdgePassAttr passAttr;
            passAttr.edge = edge;
            passAttr.srcPort = srcPort;
            passAttr.dstPort = dstPort;
            nodeAttr.dfgEdgePass.push_back(passAttr);  
        }  
        dstPort = visitNodes[routeNode].srcOutPortIdx;
        routeNode = std::make_pair(visitNodes[routeNode].srcNodeId, visitNodes[routeNode].srcInPortIdx);
    }
    // reverse the passthrough nodes from srcNode to dstNode
    std::reverse(edgeLinks.begin(), edgeLinks.end());
    // map DFG output port
    if(dstNode == nullptr){
        auto& attr = _dfgOutputAttr[edge->dstPortIdx()];
        attr.adgIOPort = mappedAdgOutPort;
        attr.routedEdgeIds.emplace(edge->id());
    }
    return true;
}


// find a routable path from dstNode to srcNode/IB by BFS
// srcNode = nullptr: IB
bool Mapping::routeDfgEdgeFromDst(DFGEdge* edge, ADGNode* srcNode, ADGNode* dstNode, const std::set<int>& dstPortRange){
    struct VisitNodeInfo{
        int dstNodeId;     // dst node ID
        int dstInPortIdx;  // input port index of dst node
        int dstOutPortIdx; // output port index of dst node
    };
    // cache visited node information, <<node-id, outport-index>, VisitNodeInfo>
    std::map<std::pair<int, int>, VisitNodeInfo> visitNodes; 
    // cache visited nodes, <node, outport-index>
    std::queue<std::pair<ADGNode*, int>> nodeQue; 
    // Breadth first search for possible routing path
    // assign the index of the output port of the srcNode
    int srcNodeOutPortIdx = edge->srcPortIdx();
    int srcNodeInPortIdx = -1;
    int mappedAdgInPort;
    std::pair<int, int> finalSrcNode; // <node-id, outport-index>
    bool success = false;
    nodeQue.push(std::make_pair(dstNode, -1));
    while(!nodeQue.empty()){
        auto queHead = nodeQue.front();
        ADGNode* adgNode = queHead.first;
        int outPortIdx = queHead.second;
        nodeQue.pop();        
        std::vector<int> inPortIdxs;
        if(outPortIdx == -1){ // dstNode
            inPortIdxs.assign(dstPortRange.begin(), dstPortRange.end());
        }else{ // intermediate node
            for(int inPortIdx : adgNode->out2ins(outPortIdx)){
                if(!routeDfgEdgePass(edge, adgNode, inPortIdx, outPortIdx, true)){ // try to find an internal link to route the edge
                    continue;
                }
                inPortIdxs.push_back(inPortIdx); // collect all available inPort 
            }   
        } 
        // search this layer of nodes
        for(int inPortIdx : inPortIdxs){
            auto elem = adgNode->input(inPortIdx);
            int nextNodeId = elem.first;
            int nextDstPort = elem.second;
            auto nextId = std::make_pair(nextNodeId, nextDstPort);
            ADGNode* nextNode = _adg->node(nextNodeId);
            auto nextNodeType = nextNode->type();                
            if(srcNode != nullptr){ // route to srcNode
                if((srcNode->id() == nextNodeId) && srcNodeOutPortIdx == nextDstPort){ // get to the srcNode                    
                    success = true;
                    finalSrcNode = nextId;
                } else if((srcNode->id() == nextNodeId) || (nextNodeType == "GPE") || // not use GPE node to route
                          (nextNodeType == "IB") || // cannot route to the srcNode through IOB
                          visitNodes.count(nextId) || // the <node-id, outport-index> already visited
                          isAdgNodeOutPortUsed(nextNodeId, nextDstPort)){ // the output port is already used
                    continue;
                }
            } else{  // route to IB
                if(nextNodeType == "IB"){
                    auto inports = nextNode->out2ins(nextDstPort);
                    std::set<int> nextSrcPortIdxs;
                    if(isDfgInputMapped(srcNodeOutPortIdx)){ // input port already mapped
                        for(auto& elem : _adg->input(_dfgInputAttr[srcNodeOutPortIdx].adgIOPort)){ 
                            if(elem.first == nextNodeId && inports.count(elem.second)){ // check node id
                                nextSrcPortIdxs.emplace(elem.second);
                                break;
                            }
                        }
                    }else{
                        nextSrcPortIdxs = inports;
                    }
                    for(int nextSrcPortIdx : nextSrcPortIdxs){
                        if(routeDfgEdgePass(edge, nextNode, nextSrcPortIdx, nextDstPort, true)){ // try to find an internal link to route the edge
                            success = true;
                            finalSrcNode = nextId;
                            srcNodeInPortIdx = nextSrcPortIdx;
                            mappedAdgInPort = nextNode->input(nextSrcPortIdx).second;
                            break;
                        }
                    }  
                    if(!success) continue;                   
                } else if((nextNodeType == "GPE") || // not use GPE node to route
                          visitNodes.count(nextId) || // the <node-id, outport-index> already visited
                          isAdgNodeOutPortUsed(nextNodeId, nextDstPort)){ // the output port is already used
                    continue;
                }
            }                
            nodeQue.push(std::make_pair(nextNode, nextDstPort)); // cache in queue
            VisitNodeInfo info;
            info.dstNodeId = adgNode->id(); //  current node ID
            info.dstInPortIdx = inPortIdx;  // input port index of src node
            info.dstOutPortIdx = outPortIdx;   //  output port index of current node 
            visitNodes[nextId] = info;
            if(success){
                break;
            }  
        }
        if(success){
            break;
        } 
    }
    if(!success){
        return false;
    }
    // route the found path
    auto routeNode = finalSrcNode;
    auto& edgeLinks = _dfgEdgeAttr[edge->id()].edgeLinks;
    int srcPort = (srcNode != nullptr)? -1 : srcNodeInPortIdx;
    while (true){
        int nodeId = routeNode.first;
        int dstPort = routeNode.second;
        // keep the DFG edge routing status
        EdgeLinkAttr edgeAttr;
        edgeAttr.srcPort = srcPort;
        edgeAttr.dstPort = dstPort;
        edgeAttr.adgNode = _adg->node(nodeId);        
        edgeLinks.push_back(edgeAttr);
        // keep the ADG Node routing status
        ADGNodeAttr& nodeAttr = _adgNodeAttr[nodeId];
        if(srcNode != nullptr && routeNode == finalSrcNode){ // srcNode
            nodeAttr.outPortUsed[dstPort] = true; // only change the output port status 
        } else if(nodeId == dstNode->id()){ // dstNode
            nodeAttr.inPortUsed[srcPort] = true;  // only change the input port status 
            break; // get to the dstNode
        } else{
            nodeAttr.inPortUsed[srcPort] = true;
            nodeAttr.outPortUsed[dstPort] = true;
            DfgEdgePassAttr passAttr;
            passAttr.edge = edge;
            passAttr.srcPort = srcPort;
            passAttr.dstPort = dstPort;
            nodeAttr.dfgEdgePass.push_back(passAttr);
        }        
        srcPort = visitNodes[routeNode].dstInPortIdx;
        routeNode = std::make_pair(visitNodes[routeNode].dstNodeId, visitNodes[routeNode].dstOutPortIdx);
    }
    // map DFG input port
    if(srcNode == nullptr){
        auto& attr = _dfgInputAttr[edge->srcPortIdx()];
        attr.adgIOPort = mappedAdgInPort;
        attr.routedEdgeIds.emplace(edge->id());
    }
    return true;
}


// find the available input ports in the dstNode to route edge
std::set<int> Mapping::availDstPorts(DFGEdge* edge, ADGNode* dstNode){
    DFGNode* dstDfgNode = _dfg->node(edge->dstId());
    int edgeDstPort = edge->dstPortIdx();
    int opereandNum = dstDfgNode->numInputs();
    GPENode* dstGpeNode = dynamic_cast<GPENode*>(dstNode);
    std::set<int> dstPortRange; // the input port index range of the dstNode
    std::vector<int> opIdxs; // operand indexes
    if(dstDfgNode->commutative()){ // operands are commutative, use all the operand indexes
        for(int opIdx = 0; opIdx < opereandNum; opIdx++){
            opIdxs.push_back(opIdx);
        }
    } else{ // operands are not commutative, use the edgeDstPort as the operand index
        opIdxs.push_back(edgeDstPort);
    }
    // select all the ports connected to available operand
    for(int opIdx : opIdxs){
        bool operandUsed = false; // if this operand is used
        auto& inPorts = dstGpeNode->operandInputs(opIdx);
        for(int inPort : inPorts){ // input ports connected to the operand with index of opIdx
            if(isAdgNodeInPortUsed(dstGpeNode->id(), inPort)){
                operandUsed = true;
                break;
            }
        }
        if(!operandUsed){ 
            for(int inPort : inPorts){ // all the ports are available
                dstPortRange.emplace(inPort);
            }                
        }
    }
    return dstPortRange;
}


// route DFG edge between srcNode and dstNode
// find a routable path from srcNode to dstNode by BFS
bool Mapping::routeDfgEdge(DFGEdge* edge, ADGNode* srcNode, ADGNode* dstNode){
    std::set<int> dstPortRange = availDstPorts(edge, dstNode); // the input port index range of the dstNode
    if(dstPortRange.empty()){ // no available input port in the dstNode
        return false;
    }
    return routeDfgEdgeFromDst(edge, srcNode, dstNode, dstPortRange);
    // if(!routeDfgEdgeFromDst(edge, srcNode, dstNode, dstPortRange)){
    //     return routeDfgEdgeFromSrc(edge, srcNode, dstNode, dstPortRange);
    // }
    // return true;
}


// route DFG edge between adgNode and IOB
// is2Input: whether connected to IB or OB
bool Mapping::routeDfgEdge(DFGEdge* edge, ADGNode* adgNode, bool is2Input){
    std::set<int> dstPortRange; // the input port index range of the dstNode
    if(!is2Input){ // route to OB
        return routeDfgEdgeFromSrc(edge, adgNode, nullptr, dstPortRange);       
    } 
    dstPortRange = availDstPorts(edge, adgNode); // the input port index range of the dstNode
    if(dstPortRange.empty()){ // no available input port in the dstNode
        return false;
    }
    return routeDfgEdgeFromDst(edge, nullptr, adgNode, dstPortRange);
}


// unroute DFG edge
void Mapping::unrouteDfgEdge(DFGEdge* edge){
    int eid = edge->id();
    if(!_dfgEdgeAttr.count(eid)) return;
    auto& edgeAttr = _dfgEdgeAttr[eid]; 
    // remove this edge from all the routed ADG links
    for(auto& edgeLink : edgeAttr.edgeLinks){
        // if(edgeLink.isLink){ // remove this edge from all the routed ADG links
        //     auto link = edgeLink.edgeLink.adgLink;
        //     if(!_adgLinkAttr.count(link->id())) continue;
        //     auto& linkEdges = _adgLinkAttr[link->id()].dfgEdges;
        //     std::remove(linkEdges.begin(), linkEdges.end(), edge);
        // } else { // remove this edge from all the passed-through ADG nodes
        auto node = edgeLink.adgNode;
        if(!_adgNodeAttr.count(node->id())) continue; 
        auto& nodeAttr = _adgNodeAttr[node->id()];
                   
        auto& nodeEdges = nodeAttr.dfgEdgePass;
        auto iter = std::remove_if(nodeEdges.begin(), nodeEdges.end(), [&](DfgEdgePassAttr& x){ return (x.edge == edge); });
        nodeEdges.erase(iter, nodeEdges.end());
        bool setInPortUsed = (edgeLink.srcPort != -1);
        bool setOutPortUsed = (edgeLink.dstPort != -1);
        for(auto& nodeEdge : nodeEdges){ // the srcPort/dstPort may be used by other edges
            if(nodeEdge.srcPort == edgeLink.srcPort){
                setInPortUsed = false;
            }
            if(nodeEdge.dstPort == edgeLink.dstPort){
                setOutPortUsed = false;
            }
            if(!setInPortUsed && !setOutPortUsed){
                break;
            }
        }
        if(setInPortUsed){
            nodeAttr.inPortUsed[edgeLink.srcPort] = false;
        }
        if(setOutPortUsed){
            nodeAttr.outPortUsed[edgeLink.dstPort] = false;
        } 
        // if(edgeLink.srcPort != -1){
        //     nodeAttr.inPortUsed[edgeLink.srcPort] = false;
        // }
        // if(edgeLink.dstPort != -1){
        //     nodeAttr.outPortUsed[edgeLink.dstPort] = false;
        // } 
        // }              
    }
    _dfgEdgeAttr.erase(eid);
    // unmap DFG IO
    if(edge->srcId() == _dfg->id()){ // connected to DFG input port
        int idx = edge->srcPortIdx();
        _dfgInputAttr[idx].routedEdgeIds.erase(eid);
    }
    if(edge->dstId() == _dfg->id()){ // connected to DFG output port
        int idx = edge->dstPortIdx();
        _dfgOutputAttr[idx].routedEdgeIds.erase(eid);
    }
}


// if succeed to map all DFG nodes
bool Mapping::success(){
    return _dfg->nodes().size() == _numNodeMapped;
}


// total/max edge length (link number)
void Mapping::getEdgeLen(int& totalLen, int& maxLen){
    int total = 0;
    int maxVal = 0;
    for(auto& elem : _dfgEdgeAttr){
        int num = elem.second.edgeLinks.size();
        total += num;
        maxVal = std::max(maxVal, num);
    }
    totalLen = total;
    maxLen = maxVal;
}


// assign DFG IO to ADG IO according to mapping result
// post-processing after mapping
void Mapping::assignDfgIO(){
    // assign input ports
    for(auto& elem : _dfg->inputEdges()){
        // IB has only one input port, connected to ADG input port
        // all the edges with the same src routing to the same IB
        int eid = *(elem.second.begin());
        auto ib = _dfgEdgeAttr[eid].edgeLinks.begin()->adgNode; // ADG IB node
        int inport = ib->input(0).second; 
        _dfgInputAttr[elem.first].adgIOPort = inport;
    }
    // assign output ports
    for(auto& elem : _dfg->outputEdges()){
        // OB has only one output port, connected to ADG output port
        auto ob = _dfgEdgeAttr[elem.second].edgeLinks.rbegin()->adgNode; // ADG OB node
        int outport = ob->output(0).begin()->second;
        _dfgOutputAttr[elem.first].adgIOPort = outport;
    }
}



// ==== tming schedule >>>>>>>>>>>>>
// // reset the timing bounds of each DFG node
// void Mapping::resetBound(){
//     // int INF = 0x3fffffff;
//     for(auto& elem : _dfg->nodes()){
//         auto& attr = _dfgNodeAttr[elem.second->id()];
//         attr.minLat = 0;
//         attr.maxLat = 0;
//     }
// }

// calculate the routing latency of each edge, not inlcuding the delay pipe
void Mapping::calEdgeRouteLat(){
    for(auto& elem : _dfgEdgeAttr){
        auto& attr = elem.second;
        int lat = 0;
        for(auto& linkAttr : attr.edgeLinks){
            if(linkAttr.adgNode->type() == "GIB"){ // edge only pass-through GIB nodes
                bool reged = dynamic_cast<GIBNode*>(linkAttr.adgNode)->outReged(linkAttr.dstPort); // output port reged
                lat += reged;
            }
        }
        attr.lat = lat;
        // attr.latNoDelay = lat;
    }
}


// calculate the DFG node latency bounds not considering the Delay components, including 
// min latency of the output ports
// ID of the DFG node with the max latency
void Mapping::latencyBound(){
    int maxLatDfg = 0; // max latency of DFG
    int maxLatNodeId;  // ID of the DFG node with the max latency
    // calculate the LOWER bounds in topological order
    for(DFGNode* node : _dfg->topoNodes()){
        int maxLat = 0; // max latency of the input ports
        int nodeId = node->id();
        int nodeOpLat = _dfg->node(nodeId)->opLatency(); // operation latency
        for(auto& elem : node->inputEdges()){
            int eid = elem.second;
            int routeLat = _dfgEdgeAttr[eid].lat; // latNoDelay;
            int srcNodeId = _dfg->edge(eid)->srcId();
            int srcNodeLat = 0; // DFG input port min latency = 0
            if(srcNodeId != _dfg->id()){ // not connected to DFG input port
                srcNodeLat = _dfgNodeAttr[srcNodeId].lat;                
            }
            int inPortLat = srcNodeLat + routeLat; 
            maxLat = std::max(maxLat, inPortLat);
        }
        // for(auto& elem : node->inputs()){
        //     int srcNodeId = elem.second.first;
        //     int srcNodeLat = 0;
        //     if(srcNodeId != _dfg->id()){ // not connected to DFG input port
        //         srcNodeLat = _dfgNodeAttr[srcNodeId].lat;                
        //     }
        //     int inPortLat = srcNodeLat; // edge no latency
        //     maxLat = std::max(maxLat, inPortLat);
        // }
        int lat = maxLat + nodeOpLat;
        _dfgNodeAttr[nodeId].lat = lat;
        if(lat >= maxLatDfg){
            maxLatDfg = lat;
            maxLatNodeId = nodeId;
        }       
    }
    // _maxLat = maxLatDfg;
    _maxLatNodeId = maxLatNodeId;
}


// schedule the latency of each DFG node based on the mapping status
// DFG node latency: output port latency
// DFG edge latency: latency from the output port of src node to the ALU Input port of dst Node, including DelayPipe 
void Mapping::latencySchedule(){
    std::set<int> scheduledNodeIds;  
    std::vector<DFGNode*> unscheduledNodes = _dfg->topoNodes();
    std::reverse(unscheduledNodes.begin(), unscheduledNodes.end()); // in reversed topological order
    // calculate the routing latency of each edge, not inlcuding the delay pipe
    calEdgeRouteLat();
    // calculate the DFG node latency bounds, finding the max-latency path 
    latencyBound();
    // schedule the DFG nodes in the max-latency path
    DFGNode* dfgNode = _dfg->node(_maxLatNodeId);
    scheduledNodeIds.emplace(dfgNode->id());
    auto iterEnd = std::remove(unscheduledNodes.begin(), unscheduledNodes.end(), dfgNode);
    // unscheduledNodes.erase(std::remove(unscheduledNodes.begin(), unscheduledNodes.end(), dfgNode), unscheduledNodes.end());
    while(dfgNode){ // until getting to the input port
        int nodeId = dfgNode->id();     
        // std::cout << "id: " << nodeId << std::endl;   
        // use the latency lower bound as the target latency
        int inPortLat = _dfgNodeAttr[nodeId].lat - _dfg->node(nodeId)->opLatency(); // input port latency
        GPENode* gpeNode = dynamic_cast<GPENode*>(_dfgNodeAttr[nodeId].adgNode); // mapped GPE node
        _dfgNodeAttr[nodeId].maxLat = inPortLat; // input port max latency 
        _dfgNodeAttr[nodeId].minLat = std::max(inPortLat - gpeNode->maxDelay(), 0); // input port min latency 
        DFGNode* srcNode = nullptr;
        for(auto& elem : dfgNode->inputEdges()){
            int eid = elem.second;
            int routeLat = _dfgEdgeAttr[eid].lat; // latNoDelay;
            int srcNodeId = _dfg->edge(eid)->srcId();
            if(srcNodeId == _dfg->id()){ // connected to DFG input port
                continue;               
            }
            int srcNodeLat = _dfgNodeAttr[srcNodeId].lat;             
            if(inPortLat == srcNodeLat + routeLat){ 
                scheduledNodeIds.emplace(srcNodeId); // latency fixed
                srcNode = _dfg->node(srcNodeId);
                iterEnd = std::remove(unscheduledNodes.begin(), iterEnd, srcNode);
                // unscheduledNodes.erase(std::remove(unscheduledNodes.begin(), unscheduledNodes.end(), srcNode), unscheduledNodes.end());
                break; // only find one path
            } 
        }
        // for(auto& elem : dfgNode->inputs()){
        //     int srcNodeId = elem.second.first;
        //     if(srcNodeId == _dfg->id()){ // connected to DFG input port
        //         continue;               
        //     }
        //     int srcNodeLat = _dfgNodeAttr[srcNodeId].lat;             
        //     if(inPortLat == srcNodeLat){ // edge no latency
        //         scheduledNodeIds.emplace(srcNodeId); // latency fixed
        //         srcNode = _dfg->node(srcNodeId);
        //         iterEnd = std::remove(unscheduledNodes.begin(), iterEnd, srcNode);
        //         // unscheduledNodes.erase(std::remove(unscheduledNodes.begin(), unscheduledNodes.end(), srcNode), unscheduledNodes.end());
        //         break; // only find one path
        //     } 
        // }
        dfgNode = srcNode;
    }
    unscheduledNodes.erase(iterEnd, unscheduledNodes.end());
    // schedule the DFG nodes not in the max-latency path
    while(!unscheduledNodes.empty()){
        for(auto iter = unscheduledNodes.begin(); iter != unscheduledNodes.end();){
            dfgNode = *iter;
            int nodeId = dfgNode->id();
            // std::cout << "id: " << nodeId << std::endl;
            int maxLat = 0x3fffffff;
            int minLat = 0;
            bool fail = false; // fail to schedule this DFG node
            bool updated = false; // maxLat/minLat UPDATED
            // schedule DFG node only if all its output nodes are already scheduled 
            // for(auto& outsPerPort : dfgNode->outputs()){
            //     for(auto& elem : outsPerPort.second){
            //         int dstNodeId = elem.first;
            //         if(dstNodeId == _dfg->id()){ // connected to DFG output port
            //             continue;
            //         }
            //         if(scheduledNodeIds.count(dstNodeId)){ // already scheduled                     
            //             maxLat = std::min(maxLat, _dfgNodeAttr[dstNodeId].maxLat);
            //             minLat = std::max(minLat, _dfgNodeAttr[dstNodeId].minLat);
            //             updated = true;
            //         }else{
            //             fail = true;
            //             break;
            //         }
            //     }
            //     if(fail) break;
            // }
            for(auto& outsPerPort : dfgNode->outputEdges()){
                for(auto& eid : outsPerPort.second){
                    int routeLat = _dfgEdgeAttr[eid].lat; // latNoDelay;
                    int dstNodeId = _dfg->edge(eid)->dstId();
                    if(dstNodeId == _dfg->id()){ // connected to DFG output port
                        continue;
                    }
                    if(scheduledNodeIds.count(dstNodeId)){ // already scheduled                     
                        maxLat = std::min(maxLat, _dfgNodeAttr[dstNodeId].maxLat - routeLat);
                        minLat = std::max(minLat, _dfgNodeAttr[dstNodeId].minLat - routeLat);
                        updated = true;
                    }else{
                        fail = true;
                        break;
                    }
                }
                if(fail) break;
            }
            if(fail){
                iter++;
                continue;
            } 
            // if all its output ports are connected to DFG output ports, keep original latency
            // otherwise, update the latency
            int targetLat = _dfgNodeAttr[nodeId].lat; 
            if(updated){
                targetLat = std::max(targetLat, std::min(maxLat, minLat));
                _dfgNodeAttr[nodeId].lat = targetLat;
            }            
            GPENode* gpeNode = dynamic_cast<GPENode*>(_dfgNodeAttr[nodeId].adgNode); // mapped GPE node
            _dfgNodeAttr[nodeId].maxLat = targetLat - _dfg->node(nodeId)->opLatency(); // input port max latency 
            _dfgNodeAttr[nodeId].minLat = std::max(_dfgNodeAttr[nodeId].maxLat - gpeNode->maxDelay(), 0); // input port min latency 
            scheduledNodeIds.emplace(nodeId); // latency fixed
            iter = unscheduledNodes.erase(iter);
        }
    }
    // calculate the latency of DFG IO
    calIOLat();
    // calculate the latency violation of each edge
    calEdgeLatVio();
}


// calculate the latency of DFG IO
void Mapping::calIOLat(){
    // DFG input port latency
    for(auto& insPerPort : _dfg->inputEdges()){
        int minLat = 0;
        int maxLat = 0x3fffffff;
        for(auto& eid : insPerPort.second){
            int routeLat = _dfgEdgeAttr[eid].lat; // latNoDelay;
            int dstNodeId = _dfg->edge(eid)->dstId();
            if(dstNodeId == _dfg->id()){ // connected to DFG output port
                continue;
            }
            minLat = std::max(minLat, _dfgNodeAttr[dstNodeId].minLat - routeLat);
            maxLat = std::min(maxLat, _dfgNodeAttr[dstNodeId].maxLat - routeLat);
        }
        int targetLat = std::min(maxLat, minLat);
        _dfgInputAttr[insPerPort.first].lat = targetLat;
    }
    int maxLat = 0;
    // DFG output port latency
    for(auto& elem : _dfg->outputEdges()){
        int eid = elem.second;
        int routeLat = _dfgEdgeAttr[eid].lat; // latNoDelay;
        int srcNodeId = _dfg->edge(eid)->srcId();
        int srcNodeLat;
        if(srcNodeId == _dfg->id()){ // connected to DFG input port
            srcNodeLat = _dfgInputAttr[_dfg->edge(eid)->srcPortIdx()].lat;               
        } else{
            srcNodeLat = _dfgNodeAttr[srcNodeId].lat;
        }
        int targetLat = srcNodeLat + routeLat;
        _dfgOutputAttr[elem.first].lat = targetLat;
        maxLat = std::max(maxLat, targetLat);
    }
    _maxLat = maxLat;
}


// calculate the latency violation of each edge
void Mapping::calEdgeLatVio(){
    int dfgSumVio = 0; // total edge latency violation
    int dfgMaxVio = 0; // max edge latency violation
    _vioDfgEdges.clear(); // DFG edges with latency violation
    for(DFGNode* node : _dfg->topoNodes()){        
        int nodeId = node->id();
        int minLat = _dfgNodeAttr[nodeId].minLat; // min latency of the input ports
        int maxLat = _dfgNodeAttr[nodeId].maxLat; // max latency of the input ports, =  latency - operation_latency
        for(auto& elem : node->inputEdges()){
            int eid = elem.second;
            int routeLat = _dfgEdgeAttr[eid].lat; // latNoDelay;
            int srcNodeId = _dfg->edge(eid)->srcId();
            int srcNodeLat;
            if(srcNodeId != _dfg->id()){ // not connected to DFG input port
                srcNodeLat = _dfgNodeAttr[srcNodeId].lat;                
            } else{ // connected to DFG input port
                srcNodeLat = _dfgInputAttr[_dfg->edge(eid)->srcPortIdx()].lat;  
            }
            _dfgEdgeAttr[eid].lat = maxLat - srcNodeLat; // including delay pipe latency    
            _dfgEdgeAttr[eid].delay = maxLat - srcNodeLat - routeLat; // delay pipe latency    
            int inPortLat = srcNodeLat + routeLat;       
            if(inPortLat < minLat){ // need to add pass node to compensate the latency gap
                int vio = minLat - inPortLat;
                _dfgEdgeAttr[eid].vio = vio;
                _vioDfgEdges.push_back(eid);
                dfgSumVio += vio;
                dfgMaxVio = std::max(dfgMaxVio, vio);
            } else {
                _dfgEdgeAttr[eid].vio = 0;
            }
        }    
    }
    _totalViolation = dfgSumVio;
    _maxViolation = dfgMaxVio;
}


// insert pass-through DFG nodes into a copy of current DFG
void Mapping::insertPassDfgNodes(DFG* newDfg){
    *newDfg = *_dfg;
    int maxNodeId = newDfg->nodes().rbegin()->first; // std::map auto sort the key
    int maxEdgeId = newDfg->edges().rbegin()->first; 
    int maxDelay = 1;
    for(auto& elem : _dfgNodeAttr){
        auto adgNode = elem.second.adgNode;
        if(adgNode){
            maxDelay = dynamic_cast<GPENode*>(adgNode)->maxDelay() + 1;
            break;
        }
    }
    int maxInsertNodesPerEdge = 2;
    for(int eid : _vioDfgEdges){ // DFG edges with latency violation
        int vio = _dfgEdgeAttr[eid].vio; // maybe add multiple nodes according to vio   
        int num = std::min(maxInsertNodesPerEdge, std::max(1, vio/maxDelay));  
        DFGEdge* e = newDfg->edge(eid);
        int srcId = e->srcId();
        int dstId = e->dstId();
        int srcPortIdx = e->srcPortIdx();
        int dstPortIdx = e->dstPortIdx();
        newDfg->delEdge(eid);
        int lastId = srcId;
        int lastPort = srcPortIdx;
        for(int i = 0; i < num; i++){
            DFGNode* newNode = new DFGNode();
            newNode->setId(++maxNodeId);
            newNode->setName("pass"+std::to_string(maxNodeId));
            newNode->setOperation("PASS");
            newDfg->addNode(newNode);
            DFGEdge* e1 = new DFGEdge(lastId, maxNodeId);
            e1->setId(++maxEdgeId); 
            e1->setSrcPortIdx(lastPort);
            e1->setDstPortIdx(0);
            newDfg->addEdge(e1);
            lastId = maxNodeId;
            lastPort = 0;
        }           
        DFGEdge* e2 = new DFGEdge(maxNodeId, dstId);
        e2->setId(++maxEdgeId); 
        e2->setSrcPortIdx(0);
        e2->setDstPortIdx(dstPortIdx);        
        newDfg->addEdge(e2);
    }
}