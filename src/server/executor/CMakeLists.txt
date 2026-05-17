target_sources(
    ${PROJECT_NAME} PRIVATE

    plan/SeqScan.cpp
    plan/InsertNode.cpp
    plan/DeleteNode.cpp
    plan/UpdateNode.cpp

    expr/BoolExpressionNode.cpp
    expr/FunctionExpressionNode.cpp
)