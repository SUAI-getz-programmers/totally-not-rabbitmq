task request
```json
{
  "id": "id-123345",
  "queue": "calcMatrixMult",
  "message":  "data:{'matrixA':[[1,2],[3,4]], 'matrixB':[[1,2],[3,4]]}",
  "result_message": "",
  "internal_status":  "",
  "worker_hash_id": "",
  "client_hash_id":  ""
}
```

simple math task
```json
{
  "a": 1,
  "b": 2
}
```

matrix determinant
```json
[
  [[1, 2], [3, 4]],
  [[1, 2], [3, 4]],
  [[1, 2], [3, 4]]
]
```

Actions:
- 0 - auth
- 1 - task request
- 2 - task result

```json
{
  "action": 1,
  "data": { // data related only to this action
    "field": "value"
}
}
```

## Register

```json
{
  "action": "register",
  "data": {
    "hash_id": "hash_id",
    "type": "worker",
    "queue": "calcMatrixMult"
  }
}
```