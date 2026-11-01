# Native menu texture binding owner

This extracts the existing Session-owned list storage, regular aliases, language
catalog, Japanese binding and attachment flag into MenuTextureBindings. Session
retains its initialized-regular-resource guard and archive/display operations;
TitleScene retains mode readiness/tasks; Display retains GPU allocations. Regular
list storage is borrowed from the earlier-declared Session owner. No universal
service interface or guest dependency is added.

Member/destruction order is unchanged inside the fixed-identity owner. Copy/move
are disabled because catalogs contain pointers into retained list storage. Actual
application construction is compiled, and no Session move is used by maintained
consumers. Raw catalog/Japanese views remain available before attachment for the
same lazy consumer guards; lookup still validates attachment/language before index.

The previous native attachment/lookup bodies are frozen verbatim in a protected
test fixture, with synthetic data only. They are a native implementation baseline,
not a new original Windows oracle. The adapter satisfies the regular-state
prerequisite; Session's original prerequisite guard remains unchanged. The new
unit compares full public catalogs, pointer equivalence classes, regular aliases,
filenames/attributes, live mutations, ignored alias declaration payloads, errors,
late partial attachment and same-shape retry behavior. All baseline outputs match.
Additional lookup checks cover all languages/indices and invalid-language/index
precedence. Five valid compiled semantic mutants are caught.

Native Session, title tasks and the actual native app translation unit compile
with C++17 O2 strict flags. Layer/provenance/readability checks pass. The unchanged
native title names table needs a separate consumer-evidence revalidation. Source
code is committed before registration/manifest update. Full native rebuild,
resource-lifecycle judges, journeys and independent review remain Root-owned.
No GPU/actor/new gameplay or complete resource/Session/campaign verdict.
