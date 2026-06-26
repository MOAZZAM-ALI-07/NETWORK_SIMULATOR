   #include "raylib.h"
#include <vector>
#include <queue>
#include <stack>
#include <string>
#include <algorithm>
#include <climits>
#include <cmath>
#include <sstream>
#include <set>
#include <fstream>

using namespace std;

// ═══════════════════════════════════════════
//  CONSTANTS
// ═══════════════════════════════════════════
const int SW = 1280, SH = 720;
const int INF = INT_MAX;
const int MAX_NODES = 20;

// Colors
Color BG        = {15, 15, 25, 255};
Color PANEL_BG  = {22, 22, 40, 255};
Color ACCENT    = {0, 200, 150, 255};
Color ACCENT2   = {0, 150, 255, 255};
Color DANGER    = {255, 80, 80, 255};
Color WARNING   = {255, 200, 0, 255};
Color NODECOL   = {40, 40, 70, 255};
Color NODESEL   = {0, 200, 150, 255};
Color EDGECOL   = {60, 60, 100, 255};
Color PATHCOL   = {0, 255, 180, 255};
Color MSTCOL    = {255, 200, 0, 255};
Color TEXTCOL   = {220, 220, 240, 255};
Color DIMTEXT   = {120, 120, 150, 255};
Color BTNBG     = {35, 35, 60, 255};
Color BTNHOV    = {50, 50, 85, 255};
Color BTNACT    = {0, 180, 130, 255};

// ═══════════════════════════════════════════
//  DATA STRUCTURES
// ═══════════════════════════════════════════
struct Node {
    Vector2 pos;
    string name;
    bool selected = false;
    bool visited  = false;
    bool inPath   = false;
    bool inMST    = false;
    float pulseT  = 0.0f;
};

struct Edge {
    int u, v, w;
    bool inPath = false;
    bool inMST  = false;
    bool highlighted = false;
};

struct LogEntry {
    string text;
    Color col;
};

// Animation step
struct AnimStep {
    int node = -1;
    int edgeU = -1, edgeV = -1;
    string msg;
    bool isPath = false;
    bool isMST  = false;
};

// Union-Find
struct UF {
    vector<int> p, r;
    UF(int n) : p(n), r(n, 0) { for(int i=0;i<n;i++) p[i]=i; }
    int find(int x){ return p[x]==x?x:p[x]=find(p[x]); }
    bool unite(int a, int b){
        a=find(a); b=find(b);
        if(a==b) return false;
        if(r[a]<r[b]) swap(a,b);
        p[b]=a; if(r[a]==r[b]) r[a]++;
        return true;
    }
};

// ═══════════════════════════════════════════
//  GLOBAL STATE
// ═══════════════════════════════════════════
vector<Node> nodes;
vector<Edge> edges;
vector<LogEntry> logs;
vector<AnimStep> animSteps;
int animIdx = 0;
float animTimer = 0.0f;
float animSpeed = 0.6f;
bool animating = false;

int selectedSrc = -1, selectedDst = -1;
int draggingNode = -1;
bool addingEdge = false;
int edgeSrc = -1;
int addEdgeWeight = 10;

// Modes
enum Mode { MODE_NORMAL, MODE_ADD_NODE, MODE_ADD_EDGE, MODE_DELETE };
Mode currentMode = MODE_NORMAL;

// Results
string resultTitle = "";
int resultCost = 0;
vector<int> resultPath;

// Panel scroll
float logScroll = 0;

// Input
bool editingWeight = false;
string weightInput = "10";

// ═══════════════════════════════════════════
//  HELPERS
// ═══════════════════════════════════════════
void addLog(const string& txt, Color col = TEXTCOL) {
    logs.push_back({txt, col});
    if((int)logs.size() > 100) logs.erase(logs.begin());
    logScroll = (float)logs.size() * 18.0f;
}

void clearHighlights() {
    for(auto& n : nodes) { n.selected=false; n.visited=false; n.inPath=false; n.inMST=false; }
    for(auto& e : edges) { e.inPath=false; e.inMST=false; e.highlighted=false; }
    resultPath.clear(); resultTitle=""; resultCost=0;
    animSteps.clear(); animIdx=0; animating=false;
}

int getNodeAt(Vector2 p) {
    for(int i=0;i<(int)nodes.size();i++){
        float dx=nodes[i].pos.x-p.x, dy=nodes[i].pos.y-p.y;
        if(dx*dx+dy*dy < 28*28) return i;
    }
    return -1;
}

int getEdge(int u, int v) {
    for(int i=0;i<(int)edges.size();i++)
        if((edges[i].u==u&&edges[i].v==v)||(edges[i].u==v&&edges[i].v==u)) return i;
    return -1;
}

// Build adjacency
vector<vector<pair<int,int>>> buildAdj() {
    int n = nodes.size();
    vector<vector<pair<int,int>>> adj(n);
    for(auto& e : edges){
        adj[e.u].push_back({e.v, e.w});
        adj[e.v].push_back({e.u, e.w});
    }
    return adj;
}

// Path reconstruction
vector<int> getPath(int src, int dst, vector<int>& par) {
    vector<int> path;
    for(int v=dst; v!=-1; v=par[v]){
        path.push_back(v);
        if(v==src) break;
    }
    if(path.empty()||path.back()!=src) return {};
    reverse(path.begin(), path.end());
    return path;
}

// ═══════════════════════════════════════════
//  ALGORITHMS
// ═══════════════════════════════════════════

// 1. Dijkstra
void runDijkstra(int src, int dst) {
    clearHighlights();
    if(src<0||dst<0||src==(int)nodes.size()||dst==(int)nodes.size()) return;
    int n=nodes.size();
    auto adj=buildAdj();
    vector<int> dist(n,INF), par(n,-1);
    priority_queue<pair<int,int>,vector<pair<int,int>>,greater<>> pq;
    dist[src]=0; pq.push({0,src});

    animSteps.clear();
    animSteps.push_back({src,-1,-1,"Start: "+nodes[src].name,false,false});

    while(!pq.empty()){
        auto[d,u]=pq.top(); pq.pop();
        if(d>dist[u]) continue;
        animSteps.push_back({u,-1,-1,"Visiting: "+nodes[u].name,false,false});
        for(auto[v,w]:adj[u]){
            if(dist[u]+w<dist[v]){
                dist[v]=dist[u]+w;
                par[v]=u;
                pq.push({dist[v],v});
                animSteps.push_back({v,u,v,"Relax: "+nodes[u].name+"->"+nodes[v].name+" ("+to_string(dist[v])+"ms)",false,false});
            }
        }
    }

    auto path=getPath(src,dst,par);
    resultPath=path;
    if(dist[dst]==INF){
        resultTitle="No path found!";
        addLog("Dijkstra: No path from "+nodes[src].name+" to "+nodes[dst].name, DANGER);
    } else {
        resultCost=dist[dst];
        resultTitle="Dijkstra: "+nodes[src].name+" -> "+nodes[dst].name;
        for(int i=0;i+1<(int)path.size();i++)
            animSteps.push_back({path[i+1],path[i],path[i+1],"Path: "+nodes[path[i]].name+"->"+nodes[path[i+1]].name,true,false});
        addLog("Dijkstra: "+nodes[src].name+" to "+nodes[dst].name+" = "+to_string(dist[dst])+"ms | Hops: "+to_string(path.size()-1), ACCENT);
    }
    animIdx=0; animating=true;
}

// 2. Bellman-Ford
void runBellmanFord(int src, int dst) {
    clearHighlights();
    int n=nodes.size();
    vector<int> dist(n,INF), par(n,-1);
    dist[src]=0;
    animSteps.clear();
    animSteps.push_back({src,-1,-1,"BF Start: "+nodes[src].name,false,false});

    for(int i=0;i<n-1;i++){
        bool updated=false;
        for(auto& e:edges){
            if(dist[e.u]!=INF&&dist[e.u]+e.w<dist[e.v]){
                dist[e.v]=dist[e.u]+e.w; par[e.v]=e.u; updated=true;
                animSteps.push_back({e.v,e.u,e.v,"Relax: "+nodes[e.u].name+"->"+nodes[e.v].name,false,false});
            }
            if(dist[e.v]!=INF&&dist[e.v]+e.w<dist[e.u]){
                dist[e.u]=dist[e.v]+e.w; par[e.u]=e.v; updated=true;
                animSteps.push_back({e.u,e.v,e.u,"Relax: "+nodes[e.v].name+"->"+nodes[e.u].name,false,false});
            }
        }
        if(!updated) break;
    }

    // Negative cycle check
    bool negCyc=false;
    for(auto& e:edges)
        if(dist[e.u]!=INF&&dist[e.u]+e.w<dist[e.v]) negCyc=true;

    auto path=getPath(src,dst,par);
    resultPath=path;
    if(negCyc){
        resultTitle="Negative cycle detected!";
        addLog("Bellman-Ford: Negative cycle!", DANGER);
    } else if(dist[dst]==INF){
        resultTitle="No path found!";
        addLog("Bellman-Ford: No path!", DANGER);
    } else {
        resultCost=dist[dst];
        resultTitle="Bellman-Ford: "+nodes[src].name+" -> "+nodes[dst].name;
        for(int i=0;i+1<(int)path.size();i++)
            animSteps.push_back({path[i+1],path[i],path[i+1],"",true,false});
        addLog("Bellman-Ford: "+nodes[src].name+" to "+nodes[dst].name+" = "+to_string(dist[dst])+"ms", ACCENT2);
    }
    animIdx=0; animating=true;
}

// 3. BFS
void runBFS(int src, int dst) {
    clearHighlights();
    int n=nodes.size();
    auto adj=buildAdj();
    vector<int> dist(n,-1), par(n,-1);
    queue<int> q;
    dist[src]=0; q.push(src);
    animSteps.clear();
    animSteps.push_back({src,-1,-1,"BFS Start: "+nodes[src].name,false,false});

    while(!q.empty()){
        int u=q.front(); q.pop();
        animSteps.push_back({u,-1,-1,"BFS Visit: "+nodes[u].name,false,false});
        for(auto[v,w]:adj[u]){
            if(dist[v]==-1){
                dist[v]=dist[u]+1; par[v]=u;
                q.push(v);
                animSteps.push_back({v,u,v,"BFS Discover: "+nodes[v].name,false,false});
            }
        }
    }

    auto path=getPath(src,dst,par);
    resultPath=path;
    if(dist[dst]==-1){
        resultTitle="BFS: No path!";
        addLog("BFS: No path from "+nodes[src].name+" to "+nodes[dst].name, DANGER);
    } else {
        resultCost=dist[dst];
        resultTitle="BFS: "+nodes[src].name+" -> "+nodes[dst].name;
        for(int i=0;i+1<(int)path.size();i++)
            animSteps.push_back({path[i+1],path[i],path[i+1],"",true,false});
        addLog("BFS: "+nodes[src].name+" to "+nodes[dst].name+" = "+to_string(dist[dst])+" hops", ACCENT);
    }
    animIdx=0; animating=true;
}

// 4. DFS
void dfsHelper(int u, int dst, vector<bool>& vis, vector<int>& par,
               vector<vector<pair<int,int>>>& adj) {
    vis[u]=true;
    animSteps.push_back({u,-1,-1,"DFS Visit: "+nodes[u].name,false,false});
    if(u==dst) return;
    for(auto[v,w]:adj[u]){
        if(!vis[v]){
            par[v]=u;
            animSteps.push_back({v,u,v,"DFS Explore: "+nodes[u].name+"->"+nodes[v].name,false,false});
            dfsHelper(v,dst,vis,par,adj);
            if(vis[dst]) return;
        }
    }
}

void runDFS(int src, int dst) {
    clearHighlights();
    int n=nodes.size();
    auto adj=buildAdj();
    vector<bool> vis(n,false);
    vector<int> par(n,-1);
    animSteps.clear();
    dfsHelper(src,dst,vis,par,adj);

    auto path=getPath(src,dst,par);
    resultPath=path;
    if(!vis[dst]){
        resultTitle="DFS: No path!";
        addLog("DFS: No path!", DANGER);
    } else {
        resultCost=(int)path.size()-1;
        resultTitle="DFS: "+nodes[src].name+" -> "+nodes[dst].name;
        for(int i=0;i+1<(int)path.size();i++)
            animSteps.push_back({path[i+1],path[i],path[i+1],"",true,false});
        addLog("DFS: "+nodes[src].name+" to "+nodes[dst].name+" = "+to_string(path.size()-1)+" hops", ACCENT);
    }
    animIdx=0; animating=true;
}

// 5. Cycle Detection
bool dfsCycle(int u, int par, vector<bool>& vis, vector<vector<pair<int,int>>>& adj) {
    vis[u]=true;
    animSteps.push_back({u,-1,-1,"Check: "+nodes[u].name,false,false});
    for(auto[v,w]:adj[u]){
        if(!vis[v]){
            if(dfsCycle(v,u,vis,adj)) return true;
        } else if(v!=par){
            animSteps.push_back({v,u,v,"CYCLE at: "+nodes[v].name,false,false});
            return true;
        }
    }
    return false;
}

void runCycleDetect() {
    clearHighlights();
    int n=nodes.size();
    auto adj=buildAdj();
    vector<bool> vis(n,false);
    animSteps.clear();
    bool found=false;
    for(int i=0;i<n;i++){
        if(!vis[i]&&dfsCycle(i,-1,vis,adj)){ found=true; break; }
    }
    resultTitle = found ? "CYCLE DETECTED! Routing Loop Found!" : "No Cycles - Network is Loop-Free";
    addLog(found?"Cycle Detection: LOOP FOUND!":"Cycle Detection: Network clean", found?DANGER:ACCENT);
    animIdx=0; animating=true;
}

// 6. Prim's MST
void runPrimsMST() {
    clearHighlights();
    int n=nodes.size();
    auto adj=buildAdj();
    vector<int> key(n,INF), par(n,-1);
    vector<bool> inMST(n,false);
    priority_queue<pair<int,int>,vector<pair<int,int>>,greater<>> pq;
    key[0]=0; pq.push({0,0});
    animSteps.clear();
    int totalCost=0;

    while(!pq.empty()){
        auto[k,u]=pq.top(); pq.pop();
        if(inMST[u]) continue;
        inMST[u]=true; totalCost+=k;
        if(par[u]!=-1)
            animSteps.push_back({u,par[u],u,"MST Edge: "+nodes[par[u]].name+"--"+nodes[u].name+" ("+to_string(k)+"ms)",false,true});
        else
            animSteps.push_back({u,-1,-1,"MST Root: "+nodes[u].name,false,true});
        for(auto[v,w]:adj[u]){
            if(!inMST[v]&&w<key[v]){
                key[v]=w; par[v]=u;
                pq.push({w,v});
            }
        }
    }
    resultTitle="Prim's MST — Total Cost: "+to_string(totalCost)+"ms";
    resultCost=totalCost;
    addLog("MST built — Total backbone cost: "+to_string(totalCost)+"ms", MSTCOL);
    animIdx=0; animating=true;
}

// ═══════════════════════════════════════════
//  DEMO NETWORK
// ═══════════════════════════════════════════
void loadDemoNetwork() {
    nodes.clear(); edges.clear(); logs.clear();
    clearHighlights();

    // 9 routers positioned nicely
    nodes.push_back({{200,180},"Core-A"});
    nodes.push_back({{420,120},"Core-B"});
    nodes.push_back({{420,280},"Dist-C"});
    nodes.push_back({{620,200},"Dist-D"});
    nodes.push_back({{620,360},"Edge-E"});
    nodes.push_back({{420,400},"Edge-F"});
    nodes.push_back({{260,360},"Edge-G"});
    nodes.push_back({{260,480},"Edge-H"});
    nodes.push_back({{500,520},"Server"});

    edges.push_back({0,1,4});
    edges.push_back({0,2,8});
    edges.push_back({1,3,11});
    edges.push_back({1,2,6});
    edges.push_back({2,3,7});
    edges.push_back({2,5,2});
    edges.push_back({3,4,9});
    edges.push_back({3,5,14});
    edges.push_back({4,5,10});
    edges.push_back({5,6,3});
    edges.push_back({6,7,5});
    edges.push_back({7,8,6});
    edges.push_back({4,8,15});

    addLog("Demo ISP Network loaded — 9 routers, 13 links", ACCENT);
}

// ═══════════════════════════════════════════
//  UI HELPERS
// ═══════════════════════════════════════════
bool DrawButton(Rectangle r, const string& label, Color bg, bool hover) {
    Color c = hover ? BTNHOV : bg;
    bool clicked = false;
    Vector2 mp = GetMousePosition();
    bool over = CheckCollisionPointRec(mp, r);
    if(over) c = BTNHOV;
    if(over && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) { c=BTNACT; clicked=true; }
    DrawRectangleRounded(r, 0.3f, 6, c);
    DrawRectangleRoundedLines(r, 0.3f, 6, over?ACCENT:EDGECOL);
    int tw = MeasureText(label.c_str(), 14);
    DrawText(label.c_str(), (int)(r.x+r.width/2-tw/2), (int)(r.y+r.height/2-7), 14, over?ACCENT:TEXTCOL);
    return clicked;
}

void DrawNodeCircle(Node& nd, int i) {
    Color outer = nd.inPath ? PATHCOL : nd.inMST ? MSTCOL : nd.visited ? ACCENT2 : nd.selected ? NODESEL : EDGECOL;
    Color inner = nd.inPath ? Color{0,80,60,255} : nd.inMST ? Color{80,60,0,255} : NODECOL;

    // Glow
    if(nd.inPath || nd.selected || nd.inMST) {
        Color glow = outer; glow.a = 60;
        DrawCircleV(nd.pos, 32, glow);
    }
    DrawCircleV(nd.pos, 24, inner);
    DrawCircleLines((int)nd.pos.x, (int)nd.pos.y, 24, outer);
    DrawCircleLines((int)nd.pos.x, (int)nd.pos.y, 25, outer);

    // Label
    string lbl = to_string(i);
    int tw = MeasureText(lbl.c_str(), 16);
    DrawText(lbl.c_str(), (int)(nd.pos.x-tw/2), (int)(nd.pos.y-8), 16, outer);

    // Name below
    int nw = MeasureText(nd.name.c_str(), 11);
    DrawText(nd.name.c_str(), (int)(nd.pos.x-nw/2), (int)(nd.pos.y+28), 11, nd.inPath?PATHCOL:DIMTEXT);
}

// ═══════════════════════════════════════════
//  MAIN
// ═══════════════════════════════════════════
int main() {
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(SW, SH, "Network Routing Simulator — CMPE 212 | DSA Project");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);

    loadDemoNetwork();
    selectedSrc = 0; selectedDst = 8;

    while(!WindowShouldClose()) {
        float dt = GetFrameTime();
        Vector2 mp = GetMousePosition();

        // ── ANIMATION TICK ──
        if(animating && !animSteps.empty()) {
            animTimer += dt;
            if(animTimer >= animSpeed) {
                animTimer = 0;
                if(animIdx < (int)animSteps.size()) {
                    auto& s = animSteps[animIdx];
                    if(s.node >= 0 && s.node < (int)nodes.size()) {
                        if(s.isPath)  nodes[s.node].inPath = true;
                        else if(s.isMST) nodes[s.node].inMST = true;
                        else nodes[s.node].visited = true;
                    }
                    if(s.edgeU >= 0 && s.edgeV >= 0) {
                        int ei = getEdge(s.edgeU, s.edgeV);
                        if(ei >= 0) {
                            if(s.isPath) edges[ei].inPath = true;
                            else if(s.isMST) edges[ei].inMST = true;
                            else edges[ei].highlighted = true;
                        }
                    }
                    if(!s.msg.empty()) addLog(s.msg, s.isPath?PATHCOL:s.isMST?MSTCOL:ACCENT2);
                    animIdx++;
                } else {
                    animating = false;
                }
            }
        }

        // Pulse effect
        for(auto& n : nodes) n.pulseT += dt;

        // ── INTERACTION ──
        // Node dragging
        if(currentMode == MODE_NORMAL) {
            if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                int ni = getNodeAt(mp);
                if(ni >= 0) {
                    // Ctrl+click = select src/dst
                    if(IsKeyDown(KEY_LEFT_CONTROL)) {
                        if(selectedSrc < 0) selectedSrc = ni;
                        else { selectedDst = ni; }
                    } else {
                        draggingNode = ni;
                    }
                    nodes[ni].selected = !nodes[ni].selected;
                }
            }
            if(IsMouseButtonDown(MOUSE_LEFT_BUTTON) && draggingNode >= 0) {
                nodes[draggingNode].pos = mp;
            }
            if(IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) draggingNode = -1;

            // Right click select src/dst
            if(IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
                int ni = getNodeAt(mp);
                if(ni >= 0) {
                    if(selectedSrc == -1 || (selectedDst != -1)) {
                        selectedSrc = ni; selectedDst = -1;
                        addLog("Source set: "+nodes[ni].name, ACCENT);
                    } else {
                        selectedDst = ni;
                        addLog("Destination set: "+nodes[ni].name, ACCENT2);
                    }
                }
            }
        }

        // Add node mode
        if(currentMode == MODE_ADD_NODE && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            // Only add if not on panel
            if(mp.x < SW - 260 && mp.x > 0 && mp.y > 50) {
                Node nd; nd.pos = mp;
                nd.name = "R"+to_string(nodes.size());
                nodes.push_back(nd);
                addLog("Added router: "+nd.name, ACCENT);
                currentMode = MODE_NORMAL;
            }
        }

        // Add edge mode
        if(currentMode == MODE_ADD_EDGE) {
            if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                int ni = getNodeAt(mp);
                if(ni >= 0) {
                    if(edgeSrc < 0) {
                        edgeSrc = ni;
                        addLog("Edge from: "+nodes[ni].name+" — now click destination", ACCENT);
                    } else if(ni != edgeSrc) {
                        int w = 0;
                        try { w = stoi(weightInput); } catch(...) { w = 10; }
                        if(w <= 0) w = 10;
                        edges.push_back({edgeSrc, ni, w});
                        addLog("Link added: "+nodes[edgeSrc].name+" <-> "+nodes[ni].name+" ("+to_string(w)+"ms)", ACCENT);
                        edgeSrc = -1;
                        currentMode = MODE_NORMAL;
                    }
                }
            }
        }

        // Delete mode
        if(currentMode == MODE_DELETE && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            int ni = getNodeAt(mp);
            if(ni >= 0) {
                addLog("Deleted router: "+nodes[ni].name, DANGER);
                // Remove edges involving ni
                edges.erase(remove_if(edges.begin(), edges.end(),
                    [ni](Edge& e){ return e.u==ni || e.v==ni; }), edges.end());
                // Fix indices
                for(auto& e : edges) {
                    if(e.u > ni) e.u--;
                    if(e.v > ni) e.v--;
                }
                nodes.erase(nodes.begin()+ni);
                if(selectedSrc >= (int)nodes.size()) selectedSrc = -1;
                if(selectedDst >= (int)nodes.size()) selectedDst = -1;
                currentMode = MODE_NORMAL;
            }
        }

        // Weight input
        if(editingWeight) {
            int k = GetCharPressed();
            while(k > 0) {
                if(k >= '0' && k <= '9' && weightInput.size() < 4)
                    weightInput += (char)k;
                k = GetCharPressed();
            }
            if(IsKeyPressed(KEY_BACKSPACE) && !weightInput.empty())
                weightInput.pop_back();
            if(IsKeyPressed(KEY_ENTER)) editingWeight = false;
        }

        // Keyboard shortcuts
        if(!editingWeight) {
            if(IsKeyPressed(KEY_D)) runDijkstra(selectedSrc, selectedDst);
            if(IsKeyPressed(KEY_B)) runBellmanFord(selectedSrc, selectedDst);
            if(IsKeyPressed(KEY_F)) runBFS(selectedSrc, selectedDst);
            if(IsKeyPressed(KEY_V)) runDFS(selectedSrc, selectedDst);
            if(IsKeyPressed(KEY_C)) runCycleDetect();
            if(IsKeyPressed(KEY_M)) runPrimsMST();
            if(IsKeyPressed(KEY_ESCAPE)) { clearHighlights(); currentMode=MODE_NORMAL; edgeSrc=-1; }
            if(IsKeyPressed(KEY_SPACE)) animating=!animating;
        }

        // ── DRAW ──
        BeginDrawing();
        ClearBackground(BG);

        // Grid dots
        for(int x=0;x<SW;x+=40) for(int y=0;y<SH;y+=40)
            DrawPixel(x,y,{40,40,60,255});

        // ── EDGES ──
        for(auto& e : edges) {
            if(e.u>=(int)nodes.size()||e.v>=(int)nodes.size()) continue;
            Vector2 a=nodes[e.u].pos, b=nodes[e.v].pos;
            Color ec = e.inPath ? PATHCOL : e.inMST ? MSTCOL : e.highlighted ? ACCENT2 : EDGECOL;
            float thick = e.inPath ? 3.5f : e.inMST ? 3.0f : e.highlighted ? 2.5f : 1.5f;

            // Glow for active edges
            if(e.inPath || e.inMST) {
                Color gl = ec; gl.a = 40;
                DrawLineEx(a, b, thick+4, gl);
            }
            DrawLineEx(a, b, thick, ec);

            // Weight label
            Vector2 mid = {(a.x+b.x)/2, (a.y+b.y)/2};
            string ws = to_string(e.w)+"ms";
            int tw = MeasureText(ws.c_str(), 10);
            DrawRectangle((int)(mid.x-tw/2-2),(int)(mid.y-7),tw+4,14,BG);
            DrawText(ws.c_str(),(int)(mid.x-tw/2),(int)(mid.y-6),10,e.inPath?PATHCOL:e.inMST?MSTCOL:DIMTEXT);
        }

        // Edge being added preview
        if(currentMode==MODE_ADD_EDGE && edgeSrc>=0)
            DrawLineEx(nodes[edgeSrc].pos, mp, 2.0f, {ACCENT.r,ACCENT.g,ACCENT.b,120});

        // ── NODES ──
        for(int i=0;i<(int)nodes.size();i++) DrawNodeCircle(nodes[i], i);

        // Src/Dst markers
        if(selectedSrc>=0 && selectedSrc<(int)nodes.size()) {
            DrawCircleLines((int)nodes[selectedSrc].pos.x,(int)nodes[selectedSrc].pos.y,30,ACCENT);
            DrawText("SRC",(int)(nodes[selectedSrc].pos.x-12),(int)(nodes[selectedSrc].pos.y-42),11,ACCENT);
        }
        if(selectedDst>=0 && selectedDst<(int)nodes.size()) {
            DrawCircleLines((int)nodes[selectedDst].pos.x,(int)nodes[selectedDst].pos.y,30,DANGER);
            DrawText("DST",(int)(nodes[selectedDst].pos.x-12),(int)(nodes[selectedDst].pos.y-42),11,DANGER);
        }

        // ── RIGHT PANEL ──
        int px = SW-260;
        DrawRectangle(px,0,260,SH,PANEL_BG);
        DrawRectangle(px,0,2,SH,ACCENT);

        // Title
        DrawText("NETWORK ROUTING",px+10,12,16,ACCENT);
        DrawText("SIMULATOR v2.0",px+10,30,13,DIMTEXT);
        DrawText("CMPE 212 | DSA Project",px+10,46,11,DIMTEXT);
        DrawLine(px,64,SW,64,EDGECOL);

        int y = 72;

        // Src/Dst selectors
        DrawText("SOURCE (Right-click node):",px+10,y,11,DIMTEXT); y+=15;
        string srcStr = selectedSrc>=0&&selectedSrc<(int)nodes.size() ? "["+to_string(selectedSrc)+"] "+nodes[selectedSrc].name : "None";
        DrawText(srcStr.c_str(),px+10,y,13,ACCENT); y+=20;
        DrawText("DESTINATION:",px+10,y,11,DIMTEXT); y+=15;
        string dstStr = selectedDst>=0&&selectedDst<(int)nodes.size() ? "["+to_string(selectedDst)+"] "+nodes[selectedDst].name : "None";
        DrawText(dstStr.c_str(),px+10,y,13,DANGER); y+=20;
        DrawLine(px,y,SW,y,EDGECOL); y+=8;

        // Algorithm buttons
        DrawText("ALGORITHMS",px+10,y,12,DIMTEXT); y+=18;
        if(DrawButton({(float)px+8,(float)y,120,26},"[D] Dijkstra",BTNBG,false)) runDijkstra(selectedSrc,selectedDst);
        if(DrawButton({(float)px+132,(float)y,118,26},"[B] Bellman-Ford",BTNBG,false)) runBellmanFord(selectedSrc,selectedDst);
        y+=32;
        if(DrawButton({(float)px+8,(float)y,120,26},"[F] BFS",BTNBG,false)) runBFS(selectedSrc,selectedDst);
        if(DrawButton({(float)px+132,(float)y,118,26},"[V] DFS",BTNBG,false)) runDFS(selectedSrc,selectedDst);
        y+=32;
        if(DrawButton({(float)px+8,(float)y,120,26},"[C] Cycle Detect",BTNBG,false)) runCycleDetect();
        if(DrawButton({(float)px+132,(float)y,118,26},"[M] Prim's MST",BTNBG,false)) runPrimsMST();
        y+=32;
        DrawLine(px,y,SW,y,EDGECOL); y+=8;

        // Result
        DrawText("RESULT",px+10,y,12,DIMTEXT); y+=18;
        if(!resultTitle.empty()) {
            // Word wrap result title
            DrawText(resultTitle.c_str(),px+10,y,12, resultTitle.find("No")==string::npos&&resultTitle.find("CYCLE")==string::npos?ACCENT:DANGER);
            y+=16;
            if(resultCost>0) {
                string cstr = "Cost: "+to_string(resultCost)+"ms | Hops: "+to_string(resultPath.size()>1?resultPath.size()-1:0);
                DrawText(cstr.c_str(),px+10,y,12,TEXTCOL);
                y+=16;
            }
        } else {
            DrawText("Run an algorithm...",px+10,y,12,DIMTEXT); y+=16;
        }
        DrawLine(px,y,SW,y,EDGECOL); y+=8;

        // Network edit tools
        DrawText("EDIT NETWORK",px+10,y,12,DIMTEXT); y+=18;
        Color aBtn = currentMode==MODE_ADD_NODE?BTNACT:BTNBG;
        Color eBtn = currentMode==MODE_ADD_EDGE?BTNACT:BTNBG;
        Color dBtn = currentMode==MODE_DELETE?BTNACT:BTNBG;
        if(DrawButton({(float)px+8,(float)y,76,24},"Add Node",aBtn,false)) {
            currentMode = currentMode==MODE_ADD_NODE?MODE_NORMAL:MODE_ADD_NODE;
            addLog("Click on canvas to place router",ACCENT);
        }
        if(DrawButton({(float)px+88,(float)y,76,24},"Add Link",eBtn,false)) {
            currentMode = currentMode==MODE_ADD_EDGE?MODE_NORMAL:MODE_ADD_EDGE;
            edgeSrc=-1;
            addLog("Click source router, then destination",ACCENT);
        }
        if(DrawButton({(float)px+168,(float)y,80,24},"Delete",dBtn,false)) {
            currentMode = currentMode==MODE_DELETE?MODE_NORMAL:MODE_DELETE;
            addLog("Click router to delete it",DANGER);
        }
        y+=30;

        // Weight input
        DrawText("Link Weight (ms):",px+10,y,11,DIMTEXT); y+=15;
        Rectangle wRect = {(float)px+8,(float)y,100,22};
        DrawRectangleRounded(wRect,0.3f,4,editingWeight?Color{30,30,55,255}:NODECOL);
        DrawRectangleRoundedLines(wRect,0.3f,4,editingWeight?ACCENT:EDGECOL);
        DrawText((weightInput+"_").c_str(),px+14,y+5,13,TEXTCOL);
        if(CheckCollisionPointRec(mp,wRect)&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) editingWeight=true;
        if(!CheckCollisionPointRec(mp,wRect)&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) editingWeight=false;
        y+=28;

        // Animation speed
        DrawText("Anim Speed:",px+10,y,11,DIMTEXT);
        if(DrawButton({(float)px+8,(float)y+14,58,22},animSpeed>0.8f?"Slow":"",BTNBG,false)) animSpeed=1.2f;
        if(DrawButton({(float)px+70,(float)y+14,58,22},"Normal",BTNBG,false)) animSpeed=0.6f;
        if(DrawButton({(float)px+132,(float)y+14,58,22},"Fast",BTNBG,false)) animSpeed=0.2f;
        y+=42;

        if(DrawButton({(float)px+8,(float)y,116,24},"Load Demo Net",BTNBG,false)) loadDemoNetwork();
        if(DrawButton({(float)px+128,(float)y,118,24},"Clear All",{50,20,20,255},false)) {
            nodes.clear(); edges.clear(); clearHighlights(); logs.clear();
            addLog("Network cleared",DIMTEXT);
        }
        y+=30;
        DrawLine(px,y,SW,y,EDGECOL); y+=6;

        // Anim status
        if(animating) {
            DrawText(("Animating... "+to_string(animIdx)+"/"+to_string(animSteps.size())).c_str(),px+10,y,11,WARNING);
            DrawText("[SPACE] Pause",px+10,y+14,11,DIMTEXT);
        } else if(!animSteps.empty()) {
            DrawText("Animation complete",px+10,y,11,ACCENT);
            DrawText("[SPACE] Replay",px+10,y+14,11,DIMTEXT);
            if(IsKeyPressed(KEY_SPACE)) { clearHighlights(); }
        }
        y+=32;
        DrawLine(px,y,SW,y,EDGECOL); y+=6;

        // LOG
        DrawText("LOG",px+10,y,12,DIMTEXT); y+=18;
        int logY = y;
        int logH = SH - logY - 10;
        BeginScissorMode(px,logY,260,logH);
        int startLog = max(0,(int)logs.size()-(logH/18));
        for(int i=startLog;i<(int)logs.size();i++) {
            string txt = logs[i].text;
            if(txt.size()>30) txt=txt.substr(0,28)+"..";
            DrawText(txt.c_str(),px+8,logY+(i-startLog)*18,11,logs[i].col);
        }
        EndScissorMode();

        // ── TOP BAR ──
        DrawRectangle(0,0,px,38,{10,10,20,200});
        DrawText("Network Routing Simulator",10,8,18,ACCENT);
        DrawText("Right-click: set SRC/DST | Drag nodes | Shortcuts: D B F V C M",10,26,11,DIMTEXT);

        // Mode indicator
        if(currentMode!=MODE_NORMAL) {
            string modeStr = currentMode==MODE_ADD_NODE?"MODE: ADD ROUTER — Click canvas":
                             currentMode==MODE_ADD_EDGE?"MODE: ADD LINK — Click 2 routers":
                             "MODE: DELETE — Click router";
            DrawRectangle(0,SH-30,px,30,{40,10,10,200});
            DrawText(modeStr.c_str(),10,SH-22,14,WARNING);
            DrawText("[ESC] Cancel",px-110,SH-22,13,DIMTEXT);
        }

        // Stats bar
        DrawText(("Routers: "+to_string(nodes.size())+" | Links: "+to_string(edges.size())).c_str(),
                 px-250, SH-22, 12, DIMTEXT);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}