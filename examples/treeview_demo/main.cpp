#include <horizon/TreeView.hpp>
#include <horizon/TreeViewItem.hpp>
#include <horizon/WaylandWindow.hpp>
#include <horizon/Window.hpp>
#include <horizon/Logger.hpp>

using namespace horizon;

int main()
{
    try
    {
        WaylandWindow app("horizon.treeview_demo", 400, 600);
        app.set_name("TreeView Demo");
        app.set_icon_name("folder-tree");

        auto wnd = std::make_unique<Window>("TreeView Toolkit Demo");
        wnd->set_size(400, 600);

        auto tree_view = std::make_unique<TreeView>();

        // Root Level
        auto root1 = std::make_unique<TreeViewItem>("user-home", "Personal");
        root1->set_bold(true);
        root1->set_expanded(true);

        auto docs = std::make_unique<TreeViewItem>("folder-documents", "Documents");
        docs->add_item(std::make_unique<TreeViewItem>("text-x-generic", "report.pdf"));
        docs->add_item(std::make_unique<TreeViewItem>("text-x-generic", "notes.txt"));
        docs->set_expanded(true);
        root1->add_item(std::move(docs));

        auto photos = std::make_unique<TreeViewItem>("folder-pictures", "Photos");
        photos->add_item(std::make_unique<TreeViewItem>("image-x-generic", "vacation.jpg"));
        photos->add_item(std::make_unique<TreeViewItem>("image-x-generic", "family.png"));
        photos->set_expanded(true);
        root1->add_item(std::move(photos));

        auto root2 = std::make_unique<TreeViewItem>("drive-harddisk", "System Disk");
        root2->set_bold(true);
        root2->set_expanded(true);

        auto usr = std::make_unique<TreeViewItem>("folder", "usr");
        auto bin = std::make_unique<TreeViewItem>("folder", "bin");
        bin->add_item(std::make_unique<TreeViewItem>("system-run", "ls"));
        bin->add_item(std::make_unique<TreeViewItem>("system-run", "cp"));
        bin->add_item(std::make_unique<TreeViewItem>("system-run", "mv"));
        bin->set_expanded(true);
        usr->add_item(std::move(bin));

        auto share = std::make_unique<TreeViewItem>("folder", "share");
        share->add_item(std::make_unique<TreeViewItem>("folder", "icons"));
        share->add_item(std::make_unique<TreeViewItem>("folder", "themes"));
        share->set_expanded(true);
        usr->add_item(std::move(share));

        usr->set_expanded(true);
        root2->add_item(std::move(usr));

        auto etc = std::make_unique<TreeViewItem>("folder", "etc");
        etc->add_item(std::make_unique<TreeViewItem>("text-x-generic", "fstab"));
        etc->add_item(std::make_unique<TreeViewItem>("text-x-generic", "hosts"));
        etc->set_expanded(true);
        root2->add_item(std::move(etc));

        tree_view->add_root_item(std::move(root1));
        tree_view->add_root_item(std::move(root2));

        // Add some many root items to trigger scrollbar
        for (int i = 0; i < 20; ++i) {
            auto extra = std::make_unique<TreeViewItem>("folder", "Extra Item " + std::to_string(i));
            extra->set_bold(true);
            tree_view->add_root_item(std::move(extra));
        }

        wnd->add_child(std::move(tree_view));
        app.set_root(std::move(wnd));
        app.run();
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << "Error: " << e.what();
        return 1;
    }
    return 0;
}
