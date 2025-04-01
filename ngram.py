import torch
from torch.nn.utils.rnn import pad_sequence

from lookahead.lookahead_cache import LookaheadCache


class SA :
    import draftretriever
    def __init__(self, path = '/home/lth/lth_code/REST_llama3/REST/datastore/datastore_chat_large.idx'):
        self.reader = self.draftretriever.Reader(index_file_path=path,)
    def get_actual_length(self, token_list):
        try:
            return token_list.index(-2)
        except ValueError:
            return len(token_list)
    def predict(self, T, k = 5000, choices = 64, long = 10):
        token_lists = []
        attn_mask = []
        for tokens in T:
            retrieved_token, _draft_attn_mask, _tree_indices, _draft_position_ids, _retrieve_indices = self.reader.search_original(tokens, k, choices, long)
            token_info = [
                (self.get_actual_length(t), t[:self.get_actual_length(t)])
                for t in retrieved_token
            ]
            sorted_indices = sorted(
                range(len(retrieved_token)),
                key=lambda i: (token_info[i][0], token_info[i][1])
            )
            sorted_tokens = [tokens[i] for i in sorted_indices]
            mask = []
            Token = []
            pos = {}
            cnt = 0


            for token in sorted_tokens:
                for t in token:
                    if t not in pos:
                        pos[t] = cnt
                        cnt += 1
                        Token.append(t)
            for token in sorted_tokens:
                for t in token:
                    while len(mask) % cnt < pos[t]:
                        mask.append(0)
                    mask.append(1)
                while len(mask) % cnt != 0:
                    mask.append(0)
            attn_mask.append(mask)
            token_lists.append(Token)
        if not token_lists:
            return torch.tensor([]), torch.tensor([])

        token_tensors = [torch.tensor(seq, dtype=torch.long) for seq in token_lists]
        mask_tensors = [torch.tensor(m, dtype=torch.float) for m in attn_mask]

        return token_tensors, mask_tensors
        # padded_tokens = pad_sequence(token_tensors, batch_first=True, padding_value=-2)
        # padded_masks = pad_sequence(mask_tensors, batch_first=True, padding_value=0.0)

        # return padded_tokens, padded_masks

class Trie:
    from lookahead.lookahead_cache import LookaheadCache
    def __init__(self):
        self.cache = LookaheadCache()
    def insert(self, token_ids, branch_length=8, final=False, mode='output', idx=-1):
        if idx == -1:
            self.cache.put(token_ids = token_ids, branch_length = branch_length, mode = mode, idx = idx)
        else:
            self.cache.stream_put(token_ids = token_ids, branch_length = branch_length, final = final, mode = mode, idx = idx)
    def predict(self, token_id_list, decoding_length=64, branch_length=8, decoding_cursors=None, mode='output', indices=None, decoding_mode='hier'):
        id_list, mask_tensor, size_list = self.cache.bat_get(
            token_id_list=token_id_list,
            decoding_length=decoding_length,
            branch_length=branch_length,
            decoding_cursors=decoding_cursors,
            mode=mode,
            indices=indices,
            decoding_mode=decoding_mode
        )
        token_lists = []
        attn_mask = []
        for token in id_list:
            mask = []
            Token = []
            pos = {}
            cnt = 0
            for t in token:
                if t not in pos:
                    pos[t] = cnt
                    cnt += 1
                    Token.append(t)
            for mask in mask_tensor:
                for P, m in mask:
                    if m == 1:
                        t = token[P]
                        while len(mask) % cnt < pos[t]:
                            mask.append(0)
                        mask.append(1)
                while len(mask) % cnt != 0:
                    mask.append(0)
            token_lists.append(Token)
            attn_mask.append(mask)
        if not token_lists:
            return torch.tensor([]), torch.tensor([])

        token_tensors = [torch.tensor(seq, dtype=torch.long) for seq in token_lists]
        mask_tensors = [torch.tensor(m, dtype=torch.float) for m in attn_mask]

        return token_tensors, mask_tensors



class GSAM:
    import sys
    sys.path.append("csrc/build")
    import GSAM_token
    def __init__(self):
        self.GSAM = GSAM_token
    def load(self, path):
        self.GSAM.load_history_gsam(path)
    def build(self, load_path, save_path):
        self.GSAM.build_history_gsam(load_path, save_path)
    def insert(self, task, token):
        self.GSAM.insert(task, token)
    def seq_end(self, task):
        self.GSAM.seq_end(task)
    def get_SAM_tokens(self, tasks, tokens, top = 10, delta = 0.5):
        token_lists = []
        cnt_lists = []
        for Idx, _ in tasks:
            task = tasks[Idx]
            token = tokens[Idx]
            result = self.GSAM.get_GSAM_tokens(task, token)
            self_token = result[0][0]
            self_cnt = result[0][1]
            history_token = result[1][0]
            history_cnt = result[1][1]
            for idx, _ in self_cnt:
                self_cnt[idx] *= delta
            for idx, _ in history_cnt:
                history_cnt[idx] *= (1 - delta)
            token_to_index = {token: idx for idx, token in enumerate(self_token)}
            for idx, token in history_token:
                position = token_to_index.get(token, None)
                if position is not None:
                    self_cnt[position] += history_cnt[id]
                else:
                    self_token.append(token)
                    self_cnt.append(history_cnt[idx])
            SUM = sum(self_cnt)
            for idx, cnt in self_cnt:
                self_cnt[idx] /= SUM
            sorted_pairs = sorted(
                zip(self_token, self_cnt),
                key=lambda pair: pair[1]
            )
            token_lists_sorted, cnt_lists_sorted = zip(*sorted_pairs)
            token_lists.append(token_lists_sorted[:top])
            cnt_lists.append(cnt_lists_sorted[:top])


        if not token_lists:
            return torch.tensor([]), torch.tensor([])


        token_tensors = [torch.tensor(seq, dtype=torch.long) for seq in token_lists]
        cnt_tensors = [torch.tensor(m, dtype=torch.float) for m in cnt_lists]

        return token_tensors[:top], cnt_tensors[:top]

    # def get_SAM_tree_tokens(self, tasks, tokens, branch = 4, long = 4, top = 10, delta = 0.5):
    #     token_lists = []
    #     cnt_lists = []
    #     for Idx, _ in tasks:
    #         task = tasks[Idx]
    #         token = tokens[Idx]
    #         result = self.GSAM.get_GSAM_tokens(task, token)
    #         self_token = result[0][0]
    #         self_cnt = result[0][1]
    #         history_token = result[1][0]
    #         history_cnt = result[1][1]
    #         for idx, _ in self_cnt:
    #             self_cnt[idx] *= delta
    #         for idx, _ in history_cnt:
    #             history_cnt[idx] *= (1 - delta)
    #         token_to_index = {token: idx for idx, token in enumerate(self_token)}
    #         for idx, token in history_token:
    #             position = token_to_index.get(token, None)
    #             if position is not None:
    #                 self_cnt[position] += history_cnt[id]
    #             else:
    #                 self_token.append(token)
    #                 self_cnt.append(history_cnt[idx])
    #         SUM = sum(self_cnt)
    #         for idx, cnt in self_cnt:
    #             self_cnt[idx] /= SUM
    #         sorted_pairs = sorted(
    #             zip(self_token, self_cnt),
    #             key=lambda pair: pair[1]
    #         )
    #         token_lists_sorted, cnt_lists_sorted = zip(*sorted_pairs)
    #         token_lists.append(token_lists_sorted[:top])
    #         cnt_lists.append(cnt_lists_sorted[:top])
    #
    #     if not token_lists:
    #         return torch.tensor([]), torch.tensor([])
    #
    #     token_tensors = [torch.tensor(seq, dtype=torch.long) for seq in token_lists]
    #     cnt_tensors = [torch.tensor(m, dtype=torch.float) for m in cnt_lists]
    #
    #     return token_tensors[:top], cnt_tensors[:top]



