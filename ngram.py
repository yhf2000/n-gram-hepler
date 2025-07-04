import torch
import sys
sys.path.append("csrc/build")
from concurrent.futures import ThreadPoolExecutor
import os

class SA :
    import draftretriever
    def __init__(self, path = '/home/lth/lth_code/REST_llama3/REST/datastore/datastore_chat_large.idx'):
        self.reader = self.draftretriever.Reader(path)
    def get_actual_length(self, token_list):
        try:
            return token_list.index(-2)
        except ValueError:
            return len(token_list)
    def predict(self, T, k = 5000, choices = 64, long = 10):
        token_lists = []
        attn_mask = []
        for tokens in T:
            retrieved_token, *_ = self.reader.search_original(tokens, k, choices, long)
            token_info = [
                # (self.get_actual_length(t), t[:self.get_actual_length(t)])
                t
                for t in retrieved_token
            ]
            sorted_indices = sorted(
                range(len(retrieved_token)),
                key=lambda i: (token_info[i][0], token_info[i][1])
            )
            sorted_tokens = [retrieved_token[i] for i in sorted_indices]
            Token = []
            mask = []
            pos = {}
            cnt = 0


            for token in sorted_tokens:
                for t in token:
                    if t not in pos and t != -2:
                        pos[t] = cnt
                        cnt += 1
                        Token.append(t)
            for token in sorted_tokens:
                mask_n = [0] * cnt
                for t in token:
                    if t == -2:
                        break
                    mask_n[pos[t]] = 1
                mask.append(mask_n)
            flattened_mask = [elem for row in mask for elem in row]
            attn_mask.append(flattened_mask)
            token_lists.append(Token)
        if not token_lists:
            return torch.tensor([]), torch.tensor([])

        token_tensors = [torch.tensor(seq, dtype=torch.long) for seq in token_lists]
        mask_tensors = [torch.tensor(m, dtype=torch.float) for m in attn_mask]
        return token_tensors, mask_tensors


class Trie:
    from lookahead.lookahead_cache import LookaheadCache
    def __init__(self):
        self.cache = self.LookaheadCache()
    def insert(self, token_ids, branch_length=8, final=False, mode='output', idx=-1):
        if idx == -1:
            self.cache.put(token_ids = token_ids, branch_length = branch_length, mode = mode, idx = idx)
        else:
            self.cache.stream_put(token_ids = token_ids, branch_length = branch_length, final = final, mode = mode, idx = idx)
    def predict(self, token_id_list, idx_list, decoding_length=64, branch_length=8, min_input_size=0, min_output_size=0, mode='mix'):
        token_lists = []
        attn_masks = []
        for Idx in range(len(token_id_list)):
            tokens, mask_orig, *_ = self.cache.hier_get(
                token_ids=token_id_list[Idx],
                decoding_length=decoding_length,
                branch_length=branch_length,
                min_input_size=min_input_size,
                min_output_size=min_output_size,
                mode=mode,
                idx=idx_list[Idx]
            )
            token_lists.append(tokens)
            attn_masks.append(mask_orig.flatten().tolist())
        if not token_lists:
            return torch.tensor([]), torch.tensor([])

        token_tensors = [torch.tensor(seq, dtype=torch.long) for seq in token_lists]
        mask_tensors = [torch.tensor(m, dtype=torch.float) for m in attn_masks]

        return token_tensors, mask_tensors



class GSAM:
    import GSAM_token
    def __init__(self):
        self.GSAM = self.GSAM_token
    def load(self, path):
        self.GSAM.load_history_gsam(path)
    def build(self, load_path, save_path):
        self.GSAM.build_history_gsam(load_path, save_path)
    def insert(self, task, token):
        self.GSAM.insert(task, token)
    def seq_end(self, task):
        self.GSAM.seq_end(task)
    def predict(self, tasks, tokens, top = 10, delta = 0.5):
        def _process_item(task, token):
            result = self.GSAM.get_SAM_tokens(task, token, top)
            self_token, self_cnt = result[0]
            history_token, history_cnt = result[1]
            self_cnt = [c * delta for c in self_cnt]
            history_cnt = [c * (1 - delta) for c in history_cnt]
            token_map = {}
            for idx, t in enumerate(self_token):
                token_map[t] = idx
            for t, cnt in zip(history_token, history_cnt):
                if t in token_map:
                    self_cnt[token_map[t]] += cnt
                else:
                    self_token.append(t)
                    self_cnt.append(cnt)
                    token_map[t] = len(self_token) - 1
            total = sum(self_cnt)
            normalized = [c / total if total != 0 else 0.0 for c in self_cnt]
            sorted_pairs = sorted(
                zip(self_token, normalized),
                key=lambda x: x[1],
                reverse=True
            )[:top]

            return (
                [pair[0] for pair in sorted_pairs],
                [pair[1] for pair in sorted_pairs]
            )

        with ThreadPoolExecutor(max_workers=min(len(tasks), os.cpu_count() * 4)) as executor:  # 限制最大线程数
            futures = [
                executor.submit(_process_item, task, token)
                for task, token in zip(tasks, tokens)
            ]

            token_lists = []
            cnt_lists = []
            for future in futures:
                try:
                    tokens, counts = future.result()
                    token_lists.append(tokens)
                    cnt_lists.append(counts)
                except Exception as e:
                    # 错误处理（记录日志或返回默认值）
                    print(f"处理失败: {str(e)}")
                    token_lists.append([])
                    cnt_lists.append([])

        if not token_lists:
            return torch.tensor([]), torch.tensor([])


        token_tensors = [torch.tensor(seq, dtype=torch.long) for seq in token_lists]
        cnt_tensors = [torch.tensor(m, dtype=torch.float) for m in cnt_lists]

        return token_tensors[:top], cnt_tensors[:top]

def main():
    test = GSAM()
    test.load("/data/share/SA_data/datastore_chat_small.bin")
    test.insert("test", [128000, 1919, 810])
    test.insert("another test", [128000, 114514, 112233])
    token, cnt = test.predict(["test", "another test", "?"], [[128000], [128000], [128000]])

    print(token)
    print(cnt)

    test = Trie()
    test.insert([128000, 1919, 810])
    test.insert([128000, 114514, 112233])
    test.insert([128000, 1919, 99884])
    token, mask = test.predict([[128000]], [-1])

    print(token)
    print(mask)

    test = SA("/data/share/REST_dataset/datastore/datastore_chat_large.idx")
    token, mask = test.predict([[128000]])

    print(token)
    print(mask)

if __name__ == "__main__":
    main()