# Coding Convention

## 1. Code Structure & Style

- **Classes/Structs**: PascalCase
  ```cpp
  class BrokerImpl {};
  struct SubscribeRequest {};
  ```

- **Functions**: camelCase
  ```cpp
  void sendMessage();
  ```

- **Variable naming** — prefix indicates scope:

  | Scope | Prefix | Example |
  |---|---|---|
  | Class member variable | `m_` | `m_clientId`, `m_isRunning` |
  | Function parameter | `p_` | `p_client`, `p_msgMutex` |
  | Local function variable | *(none)* | `int count`, `std::string line` |

  ```cpp
  class DUClient {
    std::string m_clientId;          // class member
  public:
    void connect(std::string p_host) { // function parameter
      std::string url = p_host + ":50051"; // local variable
      this->m_clientId = url;
    }
  };
  ```

- **Accessing class member variables**: always use `this->m_` prefix
  ```cpp
  this->m_clientId = p_clientId;
  ```

- **Constants, GLOBAL VARIABLES, MACROS**: ALL_CAPS
  ```cpp
  const int MAX_RETRY = 3;
  ```

- **Brace style**: opening brace on the **same line** as the declaration (K&R style). Closing brace on its own line. Short trivial one-liners may be kept inline.
  ```cpp
  // Good — multi-line body
  class Foo {
      void bar() {
          doSomething();
      }
  };

  // Good — trivial inline
  Module getModule() const override { return Module::DU; }

  // Bad — opening brace on its own line
  class Foo
  {
  }
  ```

- Limit line length to **100–120 characters**.
- One statement per line for clarity.
- Comments go **above** the code line or function they describe.
- **Indentation**: 4 spaces (no tabs).

---

## 2. Safety & Resource Management

- Prefer **RAII** — use smart pointers instead of raw `new`/`delete`.
  ```cpp
  std::unique_ptr<BrokerImpl> broker = std::make_unique<BrokerImpl>();
  std::shared_ptr<Subscriber> sub = std::make_shared<Subscriber>();
  ```

- Avoid raw owning pointers; use references or smart pointers.
- Always initialize variables before use.
- Use `const` wherever possible to express intent and prevent modification.
  ```cpp
  const std::string& getTopic() const;
  ```

---

## 3. Functions & Classes

- **One function, one purpose** — keep functions focused and small.
- Mark overridden virtual functions with `override`.
  ```cpp
  grpc::Status Publish(...) override;
  ```

- Use `explicit` for single-argument constructors to avoid implicit conversions.
  ```cpp
  explicit BrokerImpl(int port);
  ```

- Use `= delete` to disable unwanted constructors or operators.
  ```cpp
  BrokerImpl(const BrokerImpl&) = delete;
  BrokerImpl& operator=(const BrokerImpl&) = delete;
  ```

---

## 4. Modern C++

- Use `nullptr` instead of `NULL` or `0`.
  ```cpp
  int* ptr = nullptr;
  ```

- Prefer `enum class` over plain `enum` for type safety.
  ```cpp
  enum class MessageType { Text, Status };
  ```

- Use range-based `for` loops for container iteration.
  ```cpp
  for (const auto& sub : subscribers) { ... }
  ```

- Use `auto` when the type is already obvious from the right-hand side, or when spelling it out is too verbose (e.g. iterators, factory return types). Do **not** use `auto` when the type is not immediately clear from context — write it out explicitly so the reader doesn't have to guess.
  ```cpp
  // Good — type is obvious from the factory call
  auto stub = BrokerService::NewStub(channel);

  // Good — iterator type is verbose and clear from context
  auto it = subscribers.find(mailboxId);

  // Bad — reader cannot tell what type 'result' is
  auto result = process();
  ```

---

## 5. File Naming

- Use **PascalCase** for all source and header files.
  ```
  BrokerImpl.cc
  RadioUnit.h
  ThreadSafeQueue.h
  ```

- Exceptions (keep lowercase): `main.cc`, `makefile`, `jenkinsfile`

---

## 6. Folder Naming

- Use **PascalCase** for all module and library folders.
  ```
  DigitalUnit/
  RadioUnit/
  Mailbox/
  Libraries/
  ThreadSafeQueue/
  ```

- Exceptions (keep lowercase): `build/`, `docs/`, `proto/` generated output folders.

---

## 7. Branch Naming

- Use **kebab-case** with a category prefix.
  ```
  feature/short-description
  fix/short-description
  docs/short-description
  ```

- Examples:
  ```
  feature/pubsub-multi-instance
  fix/jenkins-pipeline
  docs/coding-convention
  ```

- Do not use personal names or random identifiers as branch names.

---

## 8. Merge Request (MR) Naming

- Use the format: `<type>: <short description>` (imperative, lowercase).
  ```
  feature: add pub/sub mailbox
  fix: resolve Jenkins pipeline failure
  docs: add coding convention documentation
  chore: update .gitignore for autotools artifacts
  ```

- Types:
  - `feature` — new functionality
  - `fix` — bug fix
  - `docs` — documentation only
  - `chore` — maintenance (CI, config, tooling)
  - `refactor` — code changes with no functional difference

## 9. Type Alias Naming
  - Using **camelCase** when defined a new name for data type 
  - Type aliases must be named based on their semantic meaning, not their underlying implementation.
  - *using* aliases are treated as types, not variables
  - Example:
    ```cpp
    using dbValue = std::variant<char, uint32_t, int32_t, float, std::string>;
    ```
## 10. Alphabetical order in multi-lines
  - If the list contains only one item, keep it on a single line.
  - **Example:** SUBDIRS = abc
  - If the list contains multiple items, place the assignment on its own line and use line continuations.
  - **Example:** 
  ```
  SUBDIRS = \
        abc \
        xyz \
  ```
  - Do not place the first item on the same line as the assignment when using multi-line format.
  - If subdirectories do not have dependency relationships, arrange them **alphabetically**.

## 11. Build configuration
  - Declare all executable file first.