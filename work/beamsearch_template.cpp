#include <bits/stdc++.h>

using namespace std;

namespace beam_search {

// メモリの再利用を行いつつ集合を管理するクラス
template<class T>
class ObjectPool {
    public:
        // 配列と同じようにアクセスできる
        T& operator[](int i) {
            return data_[i];
        }

        // 配列の長さを変更せずにメモリを確保する
        void reserve(size_t capacity) {
            data_.reserve(capacity);
        }

        // 要素を追加し、追加されたインデックスを返す
        int push(const T& x) {
            if (garbage_.empty()) {
                data_.push_back(x);
                return data_.size() - 1;
            } else {
                int i = garbage_.top();
                garbage_.pop();
                data_[i] = x;
                return i;
            }
        }

        // 要素を（見かけ上）削除する
        void pop(int i) {
            garbage_.push(i);
        }

        // 使用した最大のインデックス(+1)を得る
        // この値より少し大きい値をreserveすることでメモリの再割り当てがなくなる
        size_t size() {
            return data_.size();
        }

    private:
        vector<T> data_;
        stack<int> garbage_;
};

// Hash / Action / Cost / Evaluator / State は、このファイルをincludeする前に
// namespace beam_search 内で定義する。State::expand の定義はinclude後に置く。

// 展開するノードの候補を表す構造体
struct Candidate {
    Action action;
    Evaluator evaluator;
    Hash hash;
    int parent;
    Cost cost;

    Candidate(Action action, Evaluator evaluator, Hash hash, int parent, Cost cost) :
        action(action),
        evaluator(evaluator),
        hash(hash),
        parent(parent),
        cost(cost) {}
};

// ビームサーチの設定
struct Config {
    int max_turn;
    size_t beam_width;
    size_t nodes_capacity;
    uint32_t hash_map_capacity;
};

// 候補の最大コストを管理するセグメント木。外部ライブラリ不要。
class MaxSegtree {
    int size_ = 1;
    vector<pair<Cost, int>> data_;
    static pair<Cost, int> maximum(pair<Cost, int> a, pair<Cost, int> b) {
        return a.first >= b.first ? a : b;
    }
public:
    MaxSegtree() = default;
    explicit MaxSegtree(const vector<pair<Cost, int>> &values) {
        while (size_ < (int)values.size()) size_ *= 2;
        data_.assign(size_ * 2, {numeric_limits<Cost>::min(), -1});
        copy(values.begin(), values.end(), data_.begin() + size_);
        for (int i = size_ - 1; i > 0; --i) data_[i] = maximum(data_[2*i], data_[2*i+1]);
    }
    pair<Cost, int> all_prod() const { return data_[1]; }
    void set(int index, pair<Cost, int> value) {
        int i = index + size_;
        data_[i] = value;
        while ((i /= 2) > 0) data_[i] = maximum(data_[2*i], data_[2*i+1]);
    }
};

// ノードの候補から実際に追加するものを選ぶクラス
// ビーム幅の個数だけ、評価がよいものを選ぶ
// ハッシュ値が一致したものについては、評価がよいほうのみを残す
class Selector {
    public:
        explicit Selector(const Config& config) {
            hash_to_index_.reserve(config.hash_map_capacity);
            beam_width = config.beam_width;
            candidates_.reserve(beam_width);
            full_ = false;
            st_original_.resize(beam_width);
        }

        // 候補を追加する
        // ターン数最小化型の問題で、candidateによって実行可能解が得られる場合にのみ finished = true とする
        // ビーム幅分の候補をCandidateを追加したときにsegment treeを構築する
        void push(Action action, const Evaluator& evaluator, Hash hash, int parent, bool finished) {
            Cost cost = evaluator.evaluate();
            if (finished) {
                finished_candidates_.emplace_back(Candidate(action, evaluator, hash, parent, cost));
                return;
            }
            if (full_ && cost >= st_.all_prod().first) {
                // 保持しているどの候補よりもコストが小さくないとき
                return;
            }
            auto found = hash_to_index_.find(hash);

            if (found != hash_to_index_.end()) {
                int j = found->second;
                if (hash == candidates_[j].hash) {
                    // ハッシュ値が等しいものが存在しているとき
                    if (cost < candidates_[j].cost) {
                        // 更新する場合
                        candidates_[j] = Candidate(action, evaluator, hash, parent, cost);
                        if (full_) {
                            st_.set(j, {cost, j});
                        }
                    }
                    return;
                }
            }
            if (full_) {
                // segment treeが構築されている場合
                int j = st_.all_prod().second;
                hash_to_index_.erase(candidates_[j].hash);
                hash_to_index_[hash] = j;
                candidates_[j] = Candidate(action, evaluator, hash, parent, cost);
                st_.set(j, {cost, j});
            } else {
                // segment treeが構築されていない場合
                hash_to_index_[hash] = (int)candidates_.size();
                candidates_.emplace_back(Candidate(action, evaluator, hash, parent, cost));

                if (candidates_.size() == beam_width) {
                    // 保持している候補がビーム幅分になったとき
                    construct_segment_tree();
                }
            }
        }

        // 選んだ候補を返す
        const vector<Candidate>& select() const {
            return candidates_;
        }

        // 実行可能解が見つかったか
        bool have_finished() const {
            return !finished_candidates_.empty();
        }

        // 実行可能解に到達する「候補」を返す
        vector<Candidate> get_finished_candidates() const {
            return finished_candidates_;
        }

        void clear() {
            candidates_.clear();
            hash_to_index_.clear();
            finished_candidates_.clear();
            full_ = false;
        }

    private:
        size_t beam_width;
        vector<Candidate> candidates_;
        unordered_map<Hash, int> hash_to_index_;
        bool full_;
        vector<pair<Cost,int>> st_original_;
        MaxSegtree st_;
        vector<Candidate> finished_candidates_;

        void construct_segment_tree() {
            full_ = true;
            for (size_t i = 0; i < beam_width; ++i) {
                st_original_[i] = {candidates_[i].cost, i};
            }
            st_ = MaxSegtree(st_original_);
        }
};

// 探索木（二重連鎖木）のノード
struct Node {
    Action action;
    Evaluator evaluator;
    Hash hash;
    int parent, child, left, right;

    // 根のコンストラクタ
    Node(Action action, const Evaluator& evaluator, Hash hash) :
        action(action),
        evaluator(evaluator),
        hash(hash),
        parent(-1),
        child(-1),
        left(-1),
        right(-1) {}

    // 通常のコンストラクタ
    Node(const Candidate& candidate, int right) :
        action(candidate.action),
        evaluator(candidate.evaluator),
        hash(candidate.hash),
        parent(candidate.parent),
        child(-1),
        left(-1),
        right(right) {}
};

// 二重連鎖木に対する操作をまとめたクラス
class Tree {
    public:
        explicit Tree(const State& state, size_t nodes_capacity, const Node& root) :
            state_(state)
        {
            nodes_.reserve(nodes_capacity);
            root_ = nodes_.push(root);
        }

        // 状態を更新しながら深さ優先探索を行い、次のノードの候補を全てselectorに追加する
        void dfs(Selector& selector) {
            update_root();

            int v = root_;
            while (true) {
                v = move_to_leaf(v);
                state_.expand(nodes_[v].evaluator, nodes_[v].hash, v, selector);
                if (selector.have_finished() || state_.timed_out()) return;
                v = move_to_ancestor(v);
                if (v == root_) {
                    break;
                }
                v = move_to_right(v);
            }
        }

        // 根からノードvまでのパスを取得する
        vector<Action> get_path(int v) {
            // cerr << nodes_.size() << endl;

            vector<Action> path;
            while (nodes_[v].parent != -1) {
                path.push_back(nodes_[v].action);
                v = nodes_[v].parent;
            }
            reverse(path.begin(), path.end());
            return path;
        }

        // 新しいノードを追加する
        int add_leaf(const Candidate& candidate) {
            int parent = candidate.parent;
            int sibling = nodes_[parent].child;
            int v = nodes_.push(Node(candidate, sibling));

            nodes_[parent].child = v;

            if (sibling != -1) {
                nodes_[sibling].left = v;
            }
            return v;
        }

        // ノードvに子がいなかった場合、vと不要な先祖を削除する
        void remove_if_leaf(int v) {
            if (nodes_[v].child == -1) {
                remove_leaf(v);
            }
        }

        // 最も評価がよいノードを返す
        int get_best_leaf(const vector<int>& last_nodes) {
            assert(!last_nodes.empty());
            int ret = last_nodes[0];
            for (int v : last_nodes) {
                if (nodes_[v].evaluator.evaluate() < nodes_[ret].evaluator.evaluate()) {
                    ret = v;
                }
            }
            return ret;
        }

    private:
        State state_;
        ObjectPool<Node> nodes_;
        int root_;

        // 根から一本道の部分は往復しないようにする
        void update_root() {
            int child = nodes_[root_].child;
            while (child != -1 && nodes_[child].right == -1) {
                root_ = child;
                state_.move_forward(nodes_[child].action);
                child = nodes_[child].child;
            }
        }

        // ノードvの子孫で、最も左にある葉に移動する
        int move_to_leaf(int v) {
            int child = nodes_[v].child;
            while (child != -1) {
                v = child;
                state_.move_forward(nodes_[child].action);
                child = nodes_[child].child;
            }
            return v;
        }

        // ノードvの先祖で、右への分岐があるところまで移動する
        int move_to_ancestor(int v) {
            while (v != root_ && nodes_[v].right == -1) {
                state_.move_backward(nodes_[v].action);
                v = nodes_[v].parent;
            }
            return v;
        }

        // ノードvの右のノードに移動する
        int move_to_right(int v) {
            state_.move_backward(nodes_[v].action);
            v = nodes_[v].right;
            state_.move_forward(nodes_[v].action);
            return v;
        }

        // 不要になった葉を再帰的に削除する
        void remove_leaf(int v) {
            while (true) {
                int left = nodes_[v].left;
                int right = nodes_[v].right;
                if (left == -1) {
                    int parent = nodes_[v].parent;

                    if (parent == -1) {
                        cerr << "ERROR: root is removed" << endl;
                        exit(-1);
                    }
                    nodes_.pop(v);
                    nodes_[parent].child = right;
                    if (right != -1) {
                        nodes_[right].left = -1;
                        return;
                    }
                    v = parent;
                } else {
                    nodes_.pop(v);
                    nodes_[left].right = right;
                    if (right != -1) {
                        nodes_[right].left = left;
                    }
                    return;
                }
            }
        }
};

// 時間打ち切り時にも、それまでの最良手順を返す。
vector<Action> beam_search(const Config& config, State state, Node root) {
    if (root.evaluator.remaining == 0) {
        cerr << "Beam iterations: 0\n";
        return {};
    }
    Tree tree(state, config.nodes_capacity, root);
    vector<int> curr_nodes, next_nodes;
    curr_nodes.reserve(config.beam_width);
    next_nodes.reserve(config.beam_width);
    Selector selector(config);
    vector<Action> best_path;
    Cost best_cost = root.evaluator.evaluate();
    int iterations = 0;
    for (int turn = 0; turn < config.max_turn && !state.timed_out(); ++turn) {
        ++iterations;
        tree.dfs(selector);
        if (selector.have_finished()) {
            const Candidate candidate = selector.get_finished_candidates()[0];
            best_path = tree.get_path(candidate.parent);
            best_path.push_back(candidate.action);
            break;
        }
        for (const Candidate &candidate : selector.select()) {
            if (candidate.cost < best_cost) {
                best_cost = candidate.cost;
                best_path = tree.get_path(candidate.parent);
                best_path.push_back(candidate.action);
            }
        }
        if (state.timed_out() || selector.select().empty()) break;
        for (const Candidate &candidate : selector.select()) {
            next_nodes.push_back(tree.add_leaf(candidate));
            state.mark_visited(candidate.hash);
        }
        for (int v : curr_nodes) tree.remove_if_leaf(v);
        swap(curr_nodes, next_nodes);
        next_nodes.clear();
        selector.clear();
    }
    cerr << "Beam iterations: " << iterations << '\n';
    return best_path;
}

} // namespace beam_search
