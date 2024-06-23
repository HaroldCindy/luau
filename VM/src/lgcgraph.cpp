// TODO: this all probably isn't needed. Might be better
//  to just include obj color / fixed state in luaC_dump()?
//  Ah well! It's just for initial debugging!

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
    uint8_t tt;
    uint8_t memcat;
    size_t size;
    const char* name;
} Node;

typedef struct Edge
{
    GCObject* src;
    GCObject* dst;
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

    // Need to output ID as string due to ints not existing in JSON, they're really floats,
    // and our pointers are likely 64-bit.
    ss << "\"id\": \"" << (size_t)node.gco << "\"";
    ss << ", \"type\": \"" << luaT_typenames[node.tt] << "\"";
    ss << ", \"name\": \"" << (node.name ? node.name : node_gco_id(node.gco)) << "\"";
    ss << ", \"fixed\": " << (isfixed(node.gco) ? "true" : "false");

    const char *color = "unknown";
    if (isblack(node.gco))
        color = "black";
    else if (isgray(node.gco))
        color = "gray";
    else if (iswhite(node.gco))
        color = "white";

    ss << ", \"color\": \"" << color << "\"";
    ss << ", \"memcat\": " << (int)node.memcat;

    return ss.str();
}

static std::string generate_edge_attrs(const Edge &edge)
{
    std::stringstream ss;

    ss << "\"src\": \"" << (size_t)edge.src << "\"";
    ss << ", \"dst\": \"" << (size_t)edge.dst << "\"";
    if (edge.name)
        ss << ", \"name\": \"" << edge.name << "\"";
    else
        ss << ", \"name\": null";

    return ss.str();
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
        [](void* ctx, void* src, void* dst, const char* edge_name) {
            EnumContext& context = *(EnumContext*)ctx;
            context.edges.push_back({(GCObject *)src, (GCObject *)dst, edge_name});
        });

    std::ofstream ss {out, std::ios::out | std::ios::binary};
    LUAU_ASSERT(ss.is_open());

    ss << "{\n";
    ss << "  \"nodes\": [\n    ";
    bool first = true;
    for (const auto &node_iter : ctx.nodes)
    {
        if (!first)
            ss << ",\n    ";
        ss << "{" << generate_node_attrs(node_iter.second) << "}";
        first = false;
    }
    ss << "\n  ],\n";

    ss << "  \"edges\": [\n    ";
    first = true;
    for (const auto &edge : ctx.edges)
    {
        if (!first)
            ss << ",\n    ";
        ss << "{" << generate_edge_attrs(edge) << "}";
        first = false;
    }
    ss << "\n  ]\n";

    ss << "}\n";

    ss.close();
}
