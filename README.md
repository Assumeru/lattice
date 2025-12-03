# Lattice

A [sol](https://github.com/ThePhD/sol2/)-inspired framework for interacting with Lua from C++.

## Basic usage

```c++
int main() {
    // Initialize a Lua state
    lat::State state;
    // Load all libraries
    state.loadLibraries();
    // Interact with the stack in a way that will see Lua errors propagated as exceptions
    state.withStack([](lat::Stack& stack) {
        // Push a table onto the stack
        lat::TableView table = stack.pushTable();
        // Fill the table
        table["a"] = 1;
        // Make the table available as a global variable
        stack["t"] = table;
        // Pop the table off the stack
        // Note that this invalidates the TableView in much the same way reassigning std::string invalidates any of its std::string_views
        stack.pop();
        // Execute arbitrary Lua code
        stack.execute("t.a = t.a + 1");
        // Get the incremented value
        int a = stack["t"]["a"];
        // Use the library function print
        stack.execute("print(t.a)");
        // Push a lambda onto the stack
        lat::FunctionView function = stack.pushFunction([&](int b) { return a + b; });
        // Make it available as f
        stack["f"] = function;
        // Call f from Lua
        stack.execute("t.a = f(2)");
        // Create a view that returns a value when invoked from C++
        // Note that no new entry is pushed to the stack, both variables are views of the same Lua function
        lat::ReturningFunctionView<int> returningFunction = function.returning<int>();
        // Call the function from C++
        a = returningFunction(2);
        // Pop the function off the stack, invalidating both views
        stack.pop();
        // Push arbitrary Lua code onto the stack as a function
        lat::FunctionView luaFunction = stack.pushFunction("return f(...)");
        // Use the invoke method to specify a return type without going through a ReturningFunctionView
        a = luaFunction.invoke<int>(2);
    });
}
```
