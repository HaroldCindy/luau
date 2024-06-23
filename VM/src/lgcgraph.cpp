#include <string>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>

#include "lua.h"
#include "lgc.h"
#include "lgcgraph.h"


typedef struct Node
{
    GCObject* gco;
    uint8_t tag;
    uint8_t memcat;
    size_t size;
    const char* name;
} Node;

typedef struct Edge
{
    GCObject* from;
    GCObject* to;
    const char* name;
} Edge;

typedef struct EnumContext
{
    std::unordered_map<void*, Node> nodes = {};
    std::vector<Edge> edges = {};
} EnumContext;


static std::string node_gco_id(GCObject *node_gco)
{
    return "obj_" + std::to_string((size_t)node_gco);
}

static std::string generate_node_attrs(const Node &node)
{
    std::stringstream ss;

    ss << "label=\"(" << luaT_typenames[node.gco->gch.tt] << ") ";
    if (node.name)
        ss << node.name;
    else
        ss << node_gco_id(node.gco);
    ss << "\"";

    if (isfixed(node.gco))
        ss << " color=green";
    else
        ss << " color=red";

    if (isblack(node.gco))
        ss << " fill=black fontcolor=white";
    else if(isgray(node.gco))
        ss << " fill=lightgray";

    return ss.str();
}

static std::string generate_edge_attrs(const Edge &edge)
{
    std::stringstream ss;

    ss << "label=\"";
    ss << (edge.name ? edge.name : "");
    ss << "\"";

    return ss.str();
}

[[maybe_unused]]
void prune_fixed_without_unfixed_cycle(EnumContext &ctx)
{

}

void luaX_graphheap(lua_State *L, const char *out)
{
    EnumContext ctx;

    luaC_enumheap(
        L, &ctx,
        [](void* ctx, void* gco, uint8_t tt, uint8_t memcat, size_t size, const char* name) {
            EnumContext& context = *(EnumContext*)ctx;

            auto *casted_gco = ((GCObject *)gco);
            LUAU_ASSERT(casted_gco);

            context.nodes[gco] = {casted_gco, tt, memcat, size, name};
        },
        [](void* ctx, void* s, void* t, const char* edge_name) {
            EnumContext& context = *(EnumContext*)ctx;
            context.edges.push_back({(GCObject *)s, (GCObject *)t, edge_name});
        });

    std::ofstream ss {out, std::ios::out | std::ios::binary};
    LUAU_ASSERT(ss.is_open());

    ss << "digraph ObjectGraph {\n";
    ss << "  rankdir=LR;\n";
    ss << "  node[shape=box, style=filled, fillcolor=white];\n";

    for (const auto &node_iter : ctx.nodes)
    {
        const Node &node = node_iter.second;
        ss << "  " << node_gco_id(node.gco) << "[" << generate_node_attrs(node) << "]\n";
    }

    for (const auto &edge : ctx.edges)
    {
        ss << "  " << node_gco_id(edge.from) << " -> " << node_gco_id(edge.to) << "[" << generate_edge_attrs(edge) << "]\n";
    }

    ss << "}\n";

    ss.close();
}
