#pragma once

#include <ct/vt.h>
#include <cglm/struct.h>

struct Table
{
	vtVec(vec2s) verticies;
	bool isSitable;
};

struct Edge
{
	int conn;
	float weight;
};

struct DijkstraNode
{
	int prevNode;
	float rootDistance;
};

vtVec(vtVec(struct Edge))
	genGraph(vtVec(vec2s) room, vtVec(struct Table) tables, vtVec(vec2s) * vertexList);
vtVec(vtVec(struct DijkstraNode)) dijkstra(vtVec(vtVec(struct Edge)) graph);
bool pointIsReachable(vec2s a, vec2s b, vtVec(vec2s) room, vtVec(struct Table) tables);
