The unchanged da73181 verification recipe fails to compile against ca640c5 because
TaskWorkFailure::exhausted was renamed TaskWorkFailure::Exhausted by the integrated
Task ownership migration. Both new constructor test callers now use that enum.
No comparison, expected value, case generation, exclusion or mutant is changed.
Original failure: build/gamma-setup/baseline.log, clang suggests Exhausted at line 23.
