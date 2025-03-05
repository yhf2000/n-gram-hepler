import sys
sys.path.append("csrc/build")
import uvicorn
from datetime import datetime
from typing import List, Union
from fastapi import FastAPI, HTTPException
from pydantic import BaseModel
import SA_token




# 修复类名为驼峰式命名
class PredictItem(BaseModel):
    task_id: str
    candidate_num: int
    token_lists: List[List[int]]
    method: str  # 需确认是否实际使用
    dataset: str  # 需确认是否实际使用


class InitItem(BaseModel):
    task_id: str
    token_lists: List[List[int]]


# 明确字段类型，添加更精确的类型注解
class OutItem(BaseModel):
    status: str
    token: List[List[int]]  # 更明确的字段名
    prob: List[List[float]]  # 假设概率是二维列表


SA_token.chatbuild("/data/share/SA_data/datastore_chat_large.txt")
SA_token.chatbuild("/data/share/SA_data/datastore_stack_large.txt")

app = FastAPI()


# 改为POST方法更符合语义
@app.post("/predict", response_model=OutItem)
async def predict_item(item: PredictItem):
    try:
        # 实际使用中需要确认method和dataset参数的用途
        if (item.dataset == "ultra_chat"):
            token_lists, probabilities = SA_token.chat_get_SA_tokens(
                item.token_lists,
                item.candidate_num
            )
        elif (item.dataset == "the_stack_dedup"):
            token_lists, probabilities = SA_token.stack_get_SA_tokens(
                item.token_lists,
                item.candidate_num
            )

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
@app.post("/init")
async def init_datastore(item: InitItem):
    try:
        # 假设有对应的初始化方法
        SA_token.initialize_processing(item.task_id, item.token_lists)
        return {"status": "initialization successful"}
    except Exception as e:
        raise HTTPException(
            status_code=500,
            detail=f"Initialization failed: {str(e)}"
        )

if __name__ == "__main__":
    uvicorn.run("main:app", host="0.0.0.0", port=8099, reload=True)

# uvicorn FASTAPI:app --host 0.0.0.0 --port 8099