#ifndef MINER_HPP_
#define MINER_HPP_
#include "quick_pattern.h"
#include "canonical_graph.h"
#include "galois/substrate/PerThreadStorage.h"
#include "galois/substrate/SimpleLock.h"

// We provide two types of 'support': frequency and domain support.
// Frequency is used for counting, e.g. motif counting.
// Domain support, a.k.a, the minimum image-based support, is used for FSM. It has the anti-monotonic property.
typedef float MatType;
typedef unsigned Frequency;
typedef std::vector<std::vector<MatType> > Matrix;
typedef galois::GAccumulator<unsigned> UintAccu;
//typedef std::map<unsigned, unsigned> UintMap;
typedef std::unordered_map<unsigned, unsigned> UintMap;
typedef galois::substrate::PerThreadStorage<UintMap> LocalUintMap;

class Miner {
public:
	Miner() {}
	virtual ~Miner() {}
	// insert single-edge embeddings into the embedding queue (worklist)
	inline void init(EmbeddingQueueT &queue) {
		if (show) printf("\n=============================== Init ================================\n\n");
		galois::do_all(galois::iterate(graph->begin(), graph->end()),
			[&](const GNode& src) {
				#ifdef ENABLE_LABEL
				auto& src_label = graph->getData(src);
				#endif
				for (auto e : graph->edges(src)) {
					GNode dst = graph->getEdgeDst(e);
					if(src < dst) {
						#ifdef ENABLE_LABEL
						auto& dst_label = graph->getData(dst);
						#endif
						EmbeddingT new_emb;
						#ifdef ENABLE_LABEL
						new_emb.push_back(ElementType(src, 0, src_label));
						new_emb.push_back(ElementType(dst, 0, dst_label));
						#else
						new_emb.push_back(ElementType(src));
						new_emb.push_back(ElementType(dst));
						#endif
						queue.push_back(new_emb);
					}
				}
			},
			galois::chunk_size<CHUNK_SIZE>(), galois::steal(), galois::no_conflicts(),
			galois::wl<galois::worklists::PerSocketChunkFIFO<CHUNK_SIZE>>(),
			galois::loopname("Initialization")
		);
	}

protected:
	Graph *graph;
	galois::StatTimer Tconnect;
	std::vector<unsigned> degrees;
	void degree_counting() {
		degrees.resize(graph->size());
		galois::do_all(galois::iterate(graph->begin(), graph->end()),
			[&] (GNode v) {
				degrees[v] = std::distance(graph->edge_begin(v), graph->edge_end(v));
			},
			galois::loopname("DegreeCounting")
		);
	}
	inline bool is_all_connected(unsigned dst, BaseEmbedding emb) {
		unsigned n = emb.size();
		bool all_connected = true;
		for(unsigned i = 0; i < n-1; ++i) {
			unsigned from = emb.get_vertex(i);
			if (!is_connected(from, dst)) {
				all_connected = false;
				break;
			}
		}
		return all_connected;
	}
	inline bool is_connected(unsigned from, unsigned to) {
		bool connected = false;
		if (degrees[from] < degrees[to]) {
			for(auto e : graph->edges(from)) {
				GNode dst = graph->getEdgeDst(e);
				if (dst == to) {
					connected = true;
					break;
				}
			}
		} else {
			for(auto e : graph->edges(to)) {
				GNode dst = graph->getEdgeDst(e);
				if (dst == from) {
					connected = true;
					break;
				}
			}
		}
		return connected;
	}
	inline void gen_adj_matrix(unsigned n, std::vector<bool> connected, Matrix &a) {
		unsigned l = 0;
		for (unsigned i = 1; i < n; i++)
			for (unsigned j = 0; j < i; j++)
				if (connected[l++]) a[i][j] = a[j][i] = 1;
	}
	// calculate the trace of a given n*n matrix
	inline MatType trace(unsigned n, Matrix matrix) {
		MatType tr = 0;
		for (unsigned i = 0; i < n; i++) {
			tr += matrix[i][i];
		}
		return tr;
	}
	// matrix mutiplication, both a and b are n*n matrices
	Matrix product(unsigned n, Matrix a, Matrix b) {
		Matrix c(n, std::vector<MatType>(n));
		for (unsigned i = 0; i < n; ++i) { 
			for (unsigned j = 0; j < n; ++j) { 
				c[i][j] = 0; 
				for(unsigned k = 0; k < n; ++k) {
					c[i][j] += a[i][k] * b[k][j];
				}
			} 
		} 
		return c; 
	}
	// calculate the characteristic polynomial of a n*n matrix A
	inline void char_polynomial(unsigned n, Matrix &A, std::vector<MatType> &c) {
		// n is the size (num_vertices) of a graph
		// A is the adjacency matrix (n*n) of the graph
		Matrix C;
		C = A;
		for (unsigned i = 1; i <= n; i++) {
			if (i > 1) {
				for (unsigned j = 0; j < n; j ++)
					C[j][j] += c[n-i+1];
				C = product(n, A, C);
			}
			c[n-i] -= trace(n, C) / i;
		}
	}
	inline void get_connectivity(unsigned n, unsigned idx, VertexId dst, const VertexEmbedding &emb, std::vector<bool> &connected) {
		connected.push_back(true); // 0 and 1 are connected
		for (unsigned i = 2; i < n; i ++)
			for (unsigned j = 0; j < i; j++)
				if (is_connected(emb.get_vertex(i), emb.get_vertex(j)))
					connected.push_back(true);
				else connected.push_back(false);
		for (unsigned j = 0; j < n; j ++) {
			if (j == idx) connected.push_back(true);
			else if (is_connected(emb.get_vertex(j), dst))
				connected.push_back(true);
			else connected.push_back(false);
		}
	}
};

#endif // MINER_HPP_
