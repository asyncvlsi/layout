/*************************************************************************
 *
 *  Copyright (c) 2026 Yihang Yang
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *************************************************************************
 */
#include <act/act.h>
#include <act/passes.h>

#include <cstdlib>
#include <cstdio>

#include "../stk_layout.h"

namespace {

[[noreturn]] void Fail(const char *message) {
  std::fprintf(stderr, "layout refresh test failed: %s\n", message);
  std::exit(1);
}

}  // namespace

int main(int argc, char **argv) {
  Act::Init(&argc, &argv, "layout:layout.conf");
  if (argc != 3) {
    Fail("expected DESIGN.act CELLS.act");
  }

  Act design(argv[1]);
  design.Merge(argv[2]);
  design.Expand();
  Process *top = design.findProcess("test<>");
  if (!top) {
    Fail("test<> was not found");
  }

  ActCellPass cells(&design);
  cells.run(top);
  ActDynamicPass layout(&design, "stk2layout", "pass_layout.so", "layout");
  ActNetlistPass *netlist =
      dynamic_cast<ActNetlistPass *>(design.pass_find("prs2net"));
  if (!netlist) {
    Fail("prs2net was not registered");
  }
  netlist->run(top);
  layout.run(top);
  layout.run_recursive(top, 2);

  auto *raw = static_cast<ActStackLayout *>(layout.getPtrParam("raw"));
  if (!raw || !raw->getStats()) {
    Fail("layout statistics cache was not populated");
  }
  raw->setBBox(top, 10, 20, 30, 40);
  long llx = 0, lly = 0, urx = 0, ury = 0;
  if (!raw->getBBox(top, &llx, &lly, &urx, &ury)) {
    Fail("bounding-box cache was not populated");
  }

  FILE *sentinel = std::tmpfile();
  if (!sentinel) {
    Fail("could not create output sentinel");
  }
  layout.setParam("lef_file", static_cast<void *>(sentinel));
  layout.setParam("cell_file", static_cast<void *>(sentinel));
  layout.setParam("def_file", static_cast<void *>(sentinel));
  layout.setParam("area_collected", 1);

  if (layout.runcmd("design_refresh") != 1) {
    Fail("design_refresh command was refused");
  }
  if (raw->getStats()) {
    Fail("statistics cache survived design_refresh");
  }
  if (raw->getBBox(top, &llx, &lly, &urx, &ury)) {
    Fail("bounding-box cache survived design_refresh");
  }
  if (layout.getPtrParam("lef_file") || layout.getPtrParam("cell_file") ||
      layout.getPtrParam("def_file")) {
    Fail("output handle survived design_refresh");
  }
  if (layout.hasParam("area_collected")) {
    Fail("derived area marker survived design_refresh");
  }

  layout.run(top);
  layout.run_recursive(top, 2);
  if (!raw->getStats()) {
    Fail("layout pass did not rebuild state after design_refresh");
  }
  std::fclose(sentinel);
  std::puts("layout design-refresh lifecycle passed");
  return 0;
}
