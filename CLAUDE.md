# CLAUDE.md

Panduan ini untuk Claude Code (atau siapapun/apapun yang bantu implementasi) saat
kerja di repo ini. Ikuti aturan di bawah secara ketat — ini bukan saran, ini kontrak.

## Ringkasan Proyek

Communication service real-time berbasis MQTT, bagian dari sistem microservice yang lebih
besar. Service ini:
- Terima pesan chat via **REST** (Drogon)
- Publish/subscribe pesan via **MQTT** (mqtt_cpp / Boost.Asio)
- Validasi user & data lain via **gRPC** ke service lain (grpc++)

## Stack

| Layer         | Tools                                   |
|---------------|------------------------------------------|
| Compiler      | Clang + libc++ (dev), GCC di CI untuk cross-check |
| Build system  | CMake                                    |
| Package mgr   | vcpkg (manifest mode — `vcpkg.json`)     |
| HTTP/web      | Drogon                                   |
| MQTT          | mqtt_cpp (Boost.Asio-based)              |
| RPC           | grpc++ (official)                        |
| ORM           | TinyORM                                  |
| Testing       | GoogleTest + GoogleMock                  |
| C++ standard  | C++20                                    |

## Arsitektur: Hexagonal (Ports & Adapters)

```
include/
  domain/     -> entity & value object murni. TIDAK ADA dependency ke library luar.
  ports/      -> interface abstrak (pure virtual class). Kontrak, bukan implementasi.
src/
  adapters/
    mqtt/     -> implementasi port pakai mqtt_cpp
    grpc/     -> implementasi port pakai grpc++ (client & server stub)
    http/     -> Drogon controller — TIPIS, cuma parse request -> panggil use case
  app/        -> use case / orchestrator. Inject port via constructor. Ini "otak" service.
  main.cpp    -> composition root. SATU-SATUNYA tempat yang boleh `new`/`make_shared`
                 concrete adapter dan wiring semuanya jadi satu.
```

### Aturan ketat

1. **`domain/` tidak boleh import apapun** selain STL. Tidak boleh tahu soal Drogon,
   mqtt_cpp, atau grpc++.
2. **`app/` hanya boleh depend ke `ports/`**, tidak pernah ke `adapters/` secara
   langsung. Kalau butuh sesuatu dari luar, tambahkan method baru di interface port,
   bukan cast ke concrete class.
3. **`adapters/` mengimplementasikan `ports/`** dan boleh pakai library eksternal
   sebebas-bebasnya di dalam file adapter itu sendiri — tapi tidak boleh bocor
   (leak) ke file lain lewat return type yang spesifik-library (misal jangan return
   `MQTT_NS::buffer` dari fungsi yang dipanggil app layer).
4. **DI selalu manual via constructor injection**, bukan service locator, bukan
   singleton global, bukan DI container reflection-based. Kalau wiring mulai
   berantakan, pertimbangkan **Boost.DI** (compile-time, bukan runtime).
5. **Controller (adapter/http) tidak boleh punya business logic.** Kalau ada
   `if`/validasi bisnis di controller selain validasi format request, pindahkan ke
   `app/`.
6. **Satu commit/PR = satu layer berubah** sebisa mungkin. Kalau nambah fitur baru,
   urutan idealnya: tambah/ubah domain entity → tambah method di port → implement
   di adapter → pakai di use case → wiring di `main.cpp`.

## Gaya Kode

- **Strict typing**: hindari `auto` kalau tipe tidak jelas dari konteks. Hindari
  implicit conversion — pakai `explicit` di constructor single-argument.
- **Interface = pure abstract class** dengan `virtual ~X() = default;`. Nama
  interface diawali `I` (`IMessageBroker`, `IUserServiceClient`).
- **`std::shared_ptr` untuk dependency yang di-inject**, `std::unique_ptr` untuk
  ownership eksklusif internal adapter.
- **Namespace mengikuti folder**: `communication::domain`, `communication::ports`,
  `communication::adapters::mqtt`, `communication::adapters::grpc`, `communication::adapters::http`,
  `communication::app`.
- **Error handling**: pakai `std::optional`/`std::expected` (C++23 kalau
  tersedia) buat operasi yang bisa gagal secara "normal" (user tidak ketemu, dll).
  Exception hanya untuk kondisi benar-benar exceptional (koneksi database putus
  total, dll).
- **Tidak ada raw `new`/`delete`** di luar `main.cpp` composition root.
- Format pakai `.clang-format` (buat kalau belum ada, style dasar: Google atau
  LLVM dengan 4-space indent) dan jalankan `clang-tidy` sebelum commit.

## Testing

- Setiap use case di `app/` harus punya unit test dengan **semua port di-mock**
  pakai GoogleMock — tidak boleh nyalain broker MQTT atau gRPC server beneran di
  unit test.
- Integration test (opsional, terpisah target) baru boleh pakai broker/gRPC
  server beneran, taruh di `tests/integration/`.

## Yang HARUS ditanyakan sebelum implementasi

Kalau ada requirement baru yang ambigu soal di layer mana logic itu harus taruh
(domain vs app vs adapter), tanya dulu — jangan asumsi taruh di adapter cuma
karena "lebih gampang".

---

# How this repository holds itself to the rules above

Everything below this line is commentary on the contract, not an amendment to it. Where the code
departs from the contract it is written down here, with the reason and what would reverse it.

## What the build enforces, rather than asking people to remember

- **`domain/` has no dependency it could leak.** It is a CMake `INTERFACE` library with no `.cpp`
  files at all, so there is nowhere to hide a link against Drogon, gRPC or Qt. If somebody adds a
  dependency to the domain, the link line has to change in a way a reviewer can see.
- **`app/` cannot reach an adapter.** `communication_app` links `communication_domain` and nothing else; the
  adapters link *it*. An `#include "adapters/..."` from a use case does not compile.
- **Two build trees, and only one of them is expensive.** `-DCOMMUNICATION_BUILD_ADAPTERS=OFF` (the
  default) builds the domain, the use cases and their tests against nothing but the standard
  library and GoogleTest — a few seconds from cold. The adapters are the other tree, and they need
  Qt, TinyORM, gRPC, Drogon and Boost present first. The split is deliberate: the layer where a
  rule can be wrong is also the layer that is cheap to check.
- **The contract is fetched, never written here.** `bin/sync-contracts` clones kinetix-contracts
  at a pinned tag *and* commit, and copies out only `common` and `identity` — the two packages
  this service speaks. `.contracts/` is gitignored, so `git ls-files '*.proto'` stays empty, as it
  does in every other service in this estate. CMake refuses to configure if the directory is
  missing rather than falling back to anything.

## Where this repository departs from the contract above

Each of these is a deliberate choice with a measured reason. None of them is a preference.

1. **C++23, not C++20.** The contract's own error-handling rule says `std::expected` "kalau
   tersedia". It is: verified compiling with Apple clang and libc++, and GCC has had it since 13.
   Every fallible operation here returns one. Nothing else in the codebase needs C++23, so
   dropping back to C++20 means replacing `std::expected` and nothing more.

2. **The local build uses Homebrew and CMake `FetchContent`, not vcpkg.** `vcpkg.json` is present
   and lists the real dependency set, but it has not been exercised, for one reason worth knowing
   before anyone tries: **vcpkg's `tinyorm` port declares `"supports": "!(... osx ... arm ...)"`,
   and TinyORM's own CI covers Windows and Linux with no macOS job at all.** Building TinyORM from
   source on this laptop (macOS, arm64) nonetheless succeeds — measured, `libTinyOrm.0.38.1.0.dylib`
   links and installs — so the exclusion is untested rather than broken. The CI and Docker builds
   are Linux, where vcpkg's port applies normally.

3. **TinyORM is built by `FetchContent`, not found with `find_package`.** `find_package(TinyOrm)`
   fails against an install tree, and the cause is in TinyORM rather than here:
   `TinyOrmConfigVersion.cmake` locates the installed target configurations relative to
   `CMAKE_PARENT_LIST_FILE`, which during `find_package` is the *consuming* project's
   CMakeLists.txt. The glob finds nothing, the package is reported as `no-configuration`, and CMake
   refuses it while the files sit right there. Measured on TinyORM 0.38.1.

4. **Three folders under `src/adapters/` that the contract does not list.** `persistence/` holds
   the two TinyORM repositories, `system/` holds the clock, the identifier source and the settings
   reader, and `wire/` holds the one JSON shape shared by the HTTP responses and the MQTT
   payloads. Each is an adapter implementing a port, so the rule is unchanged — only the folder
   list is longer than the sketch.

5. **`IClock` and `IIdentifierSource` are faked in tests, not mocked.** Every other port is a
   `NiceMock` as the contract requires. These two are hand-written fakes (`FixedClock`,
   `CountingIdentifierSource`) because what a test needs from them is a *value it can assert on*
   later, not a record of how often they were called — and a mock that returns a fixed time is
   three lines of `ON_CALL` saying what one line of a fake says plainly.

6. **`communication::adapters::mqtt` needs `::MQTT_NS::` written out.** mqtt_cpp's namespace is also
   `mqtt`, so inside our namespace an unqualified `MQTT_NS::` resolves to
   `communication::adapters::mqtt::` and every name in the library goes missing. The contract's
   folder-follows-namespace rule is kept and the library is qualified at each use. The same
   problem does *not* arise in `communication::adapters::grpc`, because every gRPC name in those files was
   already written `::grpc::`.

## Things decided here, and why

- **A conversation is between exactly two principals, optionally about an order.** Two, not a
  list: one model then covers every pairing this business actually has — buyer and merchant, buyer
  and courier, merchant and courier — and the authorisation question stays a question with an
  answer. A group chat would need a membership table, a joining rule and a leaving rule, none of
  which anybody has asked for.

- **Communication stores no names, e-mail addresses, telephone numbers or roles.** A conversation is two
  principal ids. Who they are is identity's answer, given at the moment of asking rather than from
  a copy that has gone stale here. `IPrincipalDirectory` reads exactly one field of identity's
  response — `found` — and ignores the display name it is also handed.

- **Communication publishes no gRPC service of its own.** Nothing in this estate calls it: the two
  parties to a conversation are customers, and they arrive over HTTP and MQTT at the edge. Adding
  a communication package to the contract for nobody to call would be a contract change that buys nothing.
  Communication is a gRPC *client* of identity and that is all.

- **Authentication is a question asked of identity, per request.** `IPrincipalAuthenticator` hands
  the bearer token to identity's `ValidateToken` and receives a principal or a refusal. Communication holds
  no passwords, mints no tokens and verifies no signatures, which keeps identity the sole
  authority. There is no cache, because a cached "this token is good" outlives a revoked session,
  and revocation is the one thing a session store exists for. *If the gateway is ever confirmed to
  inject an already-verified principal header, this adapter can be swapped for a header reader
  without touching `app/` or `domain/` — that is what the port is for.*

- **Store first, then broadcast — never the other way round.** A message that went out live and
  then failed to save is one that one party has read and the other will never see again, and no
  later fetch can repair it. A broadcast that fails after a successful save costs a live update,
  nothing more, so it is *reported* (`SentMessage::broadcast`, and `delivered_live` in the HTTP
  response) and never returned as an error.

- **One MQTT topic per principal, not per conversation.** A conversation topic would need an ACL
  rule naming exactly two principals, and the broker would grow a rule every time somebody opens a
  chat. `<prefix>/principals/<principal>/messages` means one standing rule — you may subscribe to
  your own principal and nobody else's — and the authorisation question stays the one
  `domain::Conversation::Includes` answers.

- **QoS 1, and the broadcaster waits for the PUBACK.** Fire-and-forget would report a successful
  broadcast for a message the broker never took. The wait is bounded, because past the timeout the
  message is already stored.

- **A principal who is not a party sees 404, not 403.** 403 would confirm that a conversation
  exists, and these ids appear in URLs. The domain keeps `kNotAParticipant` and
  `kConversationNotFound` apart so that collapsing them is a decision taken deliberately in the
  HTTP adapter — and so the log still records which one it was.

- **Opening a conversation is idempotent, including under a race.** Two devices can both look,
  both find nothing, and both write; the unique index lets one win, and the loser looks again and
  uses what the winner stored. If the write says the row exists and the read then says it does
  not, the service reports a failure rather than handing back an id that is in nobody's database.

- **Messages are append-only.** There is no `Update` and no `Delete` on `IMessageRepository` and
  no index supporting one. The record of what a buyer and a merchant said to each other is what a
  dispute is settled on; a service that lets one side rewrite it is worse than no record.

- **A row that will not parse is `kCorruptRecord`, not `kDependencyUnavailable`.** They call for
  opposite responses: an unreachable dependency is waited out and retried, a row that will never
  parse is retried forever to no effect and needs somebody to look at the data. It answers 500,
  never 503.

- **Blocking work runs on `RequestWorkers`, not on Drogon's event loops.** Every port here blocks
  — the database call blocks, and so does the gRPC call to identity — and running them on an event
  loop stalls every other connection that loop is serving. A fixed pool, not a thread per request:
  a thread per request answers a slow database by making thousands of threads.

## Known blocker: the identity contract does not compile to C++

`identity.proto` at v1.0.16 declares, in both `GetUserProfileResponse` and
`GetMerchantInfoResponse`:

```proto
  bool has_location = 10;
  common.v1.GeoPoint location = 11;
```

protoc's C++ generator emits `bool has_location()` for the bool field *and* `bool has_location()`
for the message field's presence. They collide, and `identity.pb.h` does not compile:

```
identity.pb.h:4290:37: error: redefinition of 'has_location'
```

No other language in the estate hits this — Go, TypeScript, Python, Java, Ruby, PHP and C# all
name presence differently — which is why it has never shown up before. **Communication is the first C++
consumer of this contract.**

This is a contract change, and contracts are the source: the fix belongs in kinetix-contracts
(renaming the bool, or removing it in favour of `location`'s own presence), not in a patched copy
here.

Until it is published, exactly **five** of this repository's twenty-seven translation units fail,
and all five fail for this one reason:

```
build-full/src/adapters/contracts/identity/v1/identity.pb.cc
build-full/src/adapters/contracts/identity/v1/identity.grpc.pb.cc
src/adapters/grpc/GrpcPrincipalAuthenticator.cpp
src/adapters/grpc/GrpcPrincipalDirectory.cpp
src/main.cpp
```

The other twenty-two compile clean. If an editor is showing errors anywhere else, the editor is
misconfigured rather than the code being wrong — see the IntelliSense notes below.

The rest of the service is known to compile and link against a contract with that one field
renamed — a 4.7 MB binary that starts, reads its environment and refuses with the names of every
variable it is missing. That check was run against a copy outside the repository; nothing in this
repository is patched.

## IntelliSense, and why a fresh checkout looks broken in an editor

`CMakePresets.json` and `.vscode/` are committed because without them the C/C++ extension has
nothing to work from and paints the whole repository red. Four things matter, each measured:

- **Point the editor at `build-full`, never `build`.** The `dev` preset configures with
  `COMMUNICATION_BUILD_ADAPTERS=OFF`, so its compile database holds only `src/app` and `tests/unit` —
  seventeen entries. Every file under `src/adapters/` and `src/main.cpp` then has no entry at all,
  and IntelliSense falls back to guessing include paths for Drogon, Qt, TinyORM and gRPC. The
  `full` preset's database has all of them, with the eight `-isystem` paths those libraries need.
- **C++23 must be stated.** At the extension's default the `#include <expected>` in the domain,
  the ports and every use case is reported as a missing header, and each `std::expected` after it
  as an unknown type. One misconfiguration, hundreds of squiggles.
- **`c_cpp_properties.json` holds no comments.** The extension parses it as strict JSON, and a
  parse failure drops the entire configuration silently — which looks identical to having no
  configuration at all.
- **`WarningsAsErrors` is empty in `.clang-tidy`, and set by CI instead.** With `'*'`, an editor
  running clang-tidy live reports every style suggestion as an *error*. CI passes
  `--warnings-as-errors='*'` where that severity belongs.

To check whether the code or the editor is at fault, ask the compiler rather than the editor:

```sh
cmake --preset full
python3 - <<'PY'
import json, subprocess, shlex
root = "/path/to/kinetix-communication-service/"
db = json.load(open("build-full/compile_commands.json"))
own = [e for e in db if e["file"].startswith(root) and "/_deps/" not in e["file"]]
bad = [e["file"] for e in own
       if subprocess.run(shlex.split(e["command"]) + ["-fsyntax-only"],
                         cwd=e["directory"], capture_output=True).returncode]
print(len(own) - len(bad), "clean,", len(bad), "failing:", *bad, sep="\n")
PY
```

## Running it

```sh
# the cheap tree: domain + use cases + their tests, no external libraries
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/tests/communication_unit_tests

# the full tree: needs Qt, Drogon, gRPC and Boost, and the contract fix above
./bin/sync-contracts
cmake -S . -B build-full -G Ninja -DCMAKE_BUILD_TYPE=Release -DCOMMUNICATION_BUILD_ADAPTERS=ON
cmake --build build-full
```

`COMMUNICATION_WERROR=ON` turns the warning set into errors; CI should set it.

The process needs `COMMUNICATION_DATABASE_URL`, `COMMUNICATION_IDENTITY_ENDPOINT`, `COMMUNICATION_MQTT_HOST` and
`KINETIX_PKI_DIR`, and refuses to start without them, naming all the missing ones at once.
Everything else — timeouts, ports, the topic prefix, the worker count — has a default.

**On macOS, Homebrew's Qt ships only the SQLite driver**, so the process builds but cannot reach
Postgres there; the Linux packages (`qt6-psql`) carry QPSQL.

## Not built yet

- **CI/CD, compose, Dockerfile and the system design** — deferred deliberately, to be done for
  every new service at once.
- `tests/integration/` exists and is empty. A broker and a gRPC server belong there, never in the
  unit tests.
- A migrator. `migrations/0001_communication_tables.sql` is written and correct; nothing applies it yet.
- Read state. There are no read receipts and no unread counts, and that is an omission rather than
  a decision: adding them means a per-principal "last read message" mark, not a flag per message.
