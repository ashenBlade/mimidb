target_sources(
    ${PROJECT_NAME} PRIVATE
    SeqScan.cpp
    InsertNode.cpp
    DeleteNode.cpp
    UpdateNode.cpp

    expr/BoolExpressionNode.cpp
    expr/FunctionExpressionNode.cpp
)