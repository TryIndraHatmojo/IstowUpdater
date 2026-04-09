## Plan: Add Missing Details Handler

This plan renames the main tab to "Create/Update Assets and Sync DB Structure" and introduces a fallback form in the UI. If an `.istow` archive lacks a `.details` file, the user will be prompted to enter the missing metadata manually. This payload will be saved as a new `.details` file in the `iStowV2/tmp/` directory, and used to auto-register the ship to the local database before extracting assets.

**Steps**

1. **Modify Tab Name**: Rename "Update Assets and Sync DB Structure" to "Create/Update Assets and Sync DB Structure" in the main navigation.
2. **Update C++ Importer Model**:
   - Add a `Q_INVOKABLE bool generateDetailsAndLoad(const QVariantMap &details)` method in `IstowImporter.cpp/h`.
   - This method will take the manual user inputs, write a JSON file to `{workDir}/tmp/{fileprefix}ship.details`, assign the map to `m_shipDetails`, and emit `detailsChanged()`.
3. **Build Fallback UI**:
   - Modify the QML import flow to catch when `readDetails()` returns `false`.
   - Instead of just failing, display a form requiring inputs for: `idship`, `tipe`, `nama`, `company`, `dbid`, `fileprefix`, and `version`.
   - Once submitted, trigger `generateDetailsAndLoad(...)` to validate the state and enable the Import button.
   - _Since `registerToLocalDb()` already properly implements the insert/update logic for `LocalRepository` based on `m_shipDetails`, no overrides are needed there._

**Relevant files**

- [src/pages/MainPage.qml](src/pages/MainPage.qml) — Rename the main screen's first tab.
- [src/models/IstowImporter.h](src/models/IstowImporter.h) — Declare the new invokable method.
- [src/models/IstowImporter.cpp](src/models/IstowImporter.cpp) — Implement the JSON saving logic for the generated `.details` backup.
- [src/pages/BrowseDotIstow.qml](src/pages/BrowseDotIstow.qml) — Add the manual input form (TextFields) and tie it to the fallback state when `readDetails` fails.

**Verification**

1. Run application and verify the tab is renamed.
2. Attempt to choose a mock `.istow` file that does not contain a `.details` archive.
3. Validate that the manual input form prompts the user.
4. Fill in standard dummy data; verify `iStowV2/tmp/[fileprefix]ship.details` is generated.
5. Validate that upon clicking "Import", the missing DB row (ship) is correctly written into `shiptbl` local DB via `insertShip`.

**Decisions**

- `RegisterToLocalDb` naturally checks `LocalRepository::checkShipExists` and automatically issues `insertShip` vs `updateShip`. We only need to generate the local `.details` JSON and populate `m_shipDetails`; the existing flow will handle the DB registration transparently.
