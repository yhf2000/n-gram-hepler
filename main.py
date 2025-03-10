import sys
sys.path.append("csrc/build")
import uvicorn
from typing import List
from fastapi import FastAPI, HTTPException
from contextlib import asynccontextmanager
from pydantic import BaseModel
import Trie_token
import SA_token
import time
import draftretriever


class PredictItem(BaseModel):
    task_id: str
    candidate_num: int
    token_lists: List[List[int]]
    method: str
    dataset: List[str]


class InitItem(BaseModel):
    task_id: str
    token_lists: List[int]

class CloseItem(BaseModel):
    task_id: str

class OutItem(BaseModel):
    status: str
    token: List[List[int]]
    prob: List[List[float]]

class ReturnItem(BaseModel):
    status: str


@asynccontextmanager
async def lifespan(app: FastAPI):
    # SA_token.chatbuild("/home/lth/n-gram-hepler/datastore_chat_large.txt")
    st = time.time()
    # app.state.reader_chat = draftretriever.Reader(index_file_path='/home/lth/lth_code/REST_llama3/REST/datastore/datastore_chat_large.idx',)
    # app.state.reader_code = draftretriever.Reader(index_file_path='/home/lth/lth_code/REST_llama3/REST/datastore/datastore_stack_large.idx',)
    et = time.time()
    print("build cost: ", et - st)
    yield

app = FastAPI(lifespan=lifespan)


@app.post("/predict", response_model=OutItem)
async def predict_item(item: PredictItem):
    try:
        if (item.method == "s_sa"):
            if (item.dataset[0] == "ultra_chat"):
                token_lists = []
                probabilities = []
                for token in item.token_lists:
                    tokens, prob = app.state.reader_chat.search(token, item.candidate_num)
                    token_lists.append(tokens)
                    probabilities.append(prob)
            elif (item.dataset[0] == "the_stack_dedup"):
                token_lists = []
                probabilities = []
                for token in item.token_lists:
                    tokens, prob = app.state.reader_code.search(token, item.candidate_num)
                    token_lists.append(tokens)
                    probabilities.append(prob)
        elif (item.method == "d_trie"):
            token_lists, probabilities = Trie_token.get_Trie_tokens(item.token_lists, item.candidate_num)
        return OutItem(
            status="success",
            token=token_lists,
            prob=probabilities
        )
    except Exception as e:
        raise HTTPException(
            status="fail",
            token=[],
            prob=[]
        )


# 可选：添加初始化端点
@app.post("/init", response_model=ReturnItem)
async def init_datastore(item: InitItem):
    Trie_token.insert(item.task_id, item.token_lists)
    return ReturnItem(
        status="success",
    )


@app.post("/close", response_model=ReturnItem)
async def close_datastore(item: CloseItem):
    Trie_token.seq_end(item.task_id)
    return ReturnItem(
        status="success",
    )


if __name__ == "__main__":
    uvicorn.run("main:app", host="0.0.0.0", port=8099)

# uvicorn FASTAPI:app --host 0.0.0.0 --port 8099