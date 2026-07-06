// TranceEQ — the live UI is fully self-contained in index.html (inlined JUCE interop
// + bindings + canvas). This module is bundled only so the resource set matches the
// other APC plugins and remains importable from the design-preview harness.
import * as Juce from "./juce/index.js";
export { Juce };
