# Notebook Widget

The `Notebook` widget implements a tabbed interface, allowing the user to switch between different content panes using a header bar.

## Concepts

*   **`NotebookPage`**: A struct containing a label, an optional icon, and the `std::unique_ptr<Widget>` that represents the page's body.
*   **Header Bar**: A horizontal strip of buttons at the top (or bottom) that switches the visible page.

## Methods

*   **`add_tab(NotebookPage)`**: Adds a new tab to the notebook.
*   **`set_current_tab(int index)`**: Programmatically switches to a specific tab.

## Usage Example

```cpp
auto notebook = std::make_unique<Notebook>();

// Tab 1
auto page1_body = std::make_unique<Label>("This is Page 1");
notebook->add_tab(NotebookPage("General", std::move(page1_body)));

// Tab 2
auto page2_body = std::make_unique<Label>("This is Page 2");
notebook->add_tab(NotebookPage("Advanced", std::move(page2_body)));

notebook->set_current_tab(0);
```
