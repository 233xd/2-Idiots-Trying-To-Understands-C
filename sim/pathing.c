#define GLT_IMPL
#include <cglm/struct.h>
#include <ct/vt.h>

#include "intercect.h"
#include "pathing.h"
#include <math.h>

bool pointIsReachable(vec2s a, vec2s b, vtVec(vec2s) room, vtVec(struct Table) tables)
{
	for (int i = 0; i < vtLen(room); i++)
	{
		int edge[2] = {i, (i + 1) % vtLen(room)};
		if (glms_vec2_eqv(room[edge[0]], a) && glms_vec2_eqv(room[edge[1]], a) &&
			glms_vec2_eqv(room[edge[0]], b) && glms_vec2_eqv(room[edge[1]], b))
			continue;

		if (doIntersect(a, b, room[edge[0]], room[edge[1]]))
		{
			return false;
		}
	}

	for (int polyIndex = 0; polyIndex < vtLen(tables); polyIndex++)
	{
		vtVec(vec2s) poly = tables[polyIndex].verticies;

		for (int i = 0; i < vtLen(poly); i++)
		{
			for (int j = i + 1; j < vtLen(poly); j++)
			{
				int edge[2] = {i, j};

				if (glms_vec2_eqv(poly[edge[0]], a) ||
					glms_vec2_eqv(poly[edge[1]], a) ||
					glms_vec2_eqv(poly[edge[0]], b) ||
					glms_vec2_eqv(poly[edge[1]], b))
					continue;

				if (doIntersect(a, b, poly[edge[0]], poly[edge[1]]))
					return false;
			}
		}
	}

	return true;
}

vtVec(vtVec(struct Edge))
	genGraph(vtVec(vec2s) room, vtVec(struct Table) tables, vtVec(vec2s) * vertexList)
{
	vec2s* verticies = vtInit(vec2s, 0);
	for (int i = 0; i < vtLen(room); i++)
	{
		vtPush(&verticies, room[i]);
	}
	for (int i = 0; i < vtLen(tables); i++)
	{
		for (int j = 0; j < vtLen(tables[i].verticies); j++)
		{
			vtPush(&verticies, tables[i].verticies[j]);
		}
	}

	int polyOffsets[vtLen(tables)];
	int counter = 0;

	for (int i = 0; i < vtLen(tables); i++)
	{
		counter += vtLen(tables[i].verticies);
		polyOffsets[i] = counter;
	}

	vtVec(vtVec(struct Edge)) ret = vtInit(vtVec(struct Edge), vtLen(verticies));
	for (int i = 0; i < vtLen(verticies); i++)
		ret[i] = vtInit(struct Edge, 0);

	for (int a = 0; a < vtLen(verticies); a++)
		for (int b = a + 1; b < vtLen(verticies); b++)
		{
			for (int i = 0; i < vtLen(room); i++)
			{
				int edge[2] = {i, (i + 1) % vtLen(room)};
				if (edge[0] == a || edge[0] == b || edge[1] == a || edge[1] == b)
					continue;

				if (doIntersect(verticies[a], verticies[b], room[edge[0]],
					    room[edge[1]]))
				{
					goto LOOP_END;
				}
			}

			for (int polyIndex = 0; polyIndex < vtLen(tables); polyIndex++)
			{
				vtVec(vec2s) poly = tables[polyIndex].verticies;

				for (int i = 0; i < vtLen(poly); i++)
				{
					for (int j = i + 1; j < vtLen(poly); j++)
					{
						int edge[2] = {i, j};

						if (edge[0] + polyOffsets[polyIndex] == a ||
							edge[0] + polyOffsets[polyIndex] == b ||
							edge[1] + polyOffsets[polyIndex] == a ||
							edge[1] + polyOffsets[polyIndex] == b)
							continue;

						if (doIntersect(verticies[a], verticies[b],
							    poly[edge[0]], poly[edge[1]]))
							goto LOOP_END;
					}
				}
			}

			float dist = glms_vec2_distance(verticies[a], verticies[b]);

			vtPush(&ret[a], ((struct Edge){b, dist}));
			vtPush(&ret[b], ((struct Edge){a, dist}));

		LOOP_END:;
		}

	*vertexList = verticies;
	return ret;
}

void dijkstraVisit(vtVec(vtVec(struct Edge)) graph, int root, vtVec(struct DijkstraNode) nodes,
	vtVec(bool) visited)
{
	visited[root] = true;

	for (int i = 0; i < vtLen(graph[root]); i++)
	{
		struct Edge edge = graph[root][i];

		if (nodes[edge.conn].rootDistance > nodes[root].rootDistance + edge.weight)
		{
			nodes[edge.conn].rootDistance = nodes[root].rootDistance + edge.weight;
			nodes[edge.conn].prevNode = root;
		}
	}

	for (int i = 0; i < vtLen(graph[root]); i++)
	{
		struct Edge edge = graph[root][i];

		if (!visited[edge.conn])
			dijkstraVisit(graph, edge.conn, nodes, visited);
	}
}

vtVec(vtVec(struct DijkstraNode)) dijkstra(vtVec(vtVec(struct Edge)) graph)
{
	vtVec(struct DijkstraNode) nodes = vtInit(struct DijkstraNode, vtLen(graph));
	vtVec(bool) visited = vtInit(bool, vtLen(graph));
	vtVec(vtVec(struct DijkstraNode)) ret = vtInit(vtVec(struct DijkstraNode), vtLen(graph));

	for (int i = 0; i < vtLen(graph); i++)
		ret[i] = vtInit(struct DijkstraNode, 0);

	for (int currSrc = 0; currSrc < vtLen(graph); currSrc++)
	{
		for (int i = 0; i < vtLen(graph); i++)
		{
			nodes[i] = (struct DijkstraNode){
				.prevNode = -1,
				.rootDistance = INFINITY,
			};
			visited[i] = false;
		}

		nodes[currSrc].rootDistance = 0;
		dijkstraVisit(graph, currSrc, nodes, visited);

		vtPushArr(&ret[currSrc], nodes, vtLen(nodes));
	}

	vtFree(visited);
	vtFree(nodes);
	return ret;
}
