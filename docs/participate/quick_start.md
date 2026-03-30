---
title: Quick Start to Develop OpenMQTTGateway
permalink: /participate/quick_start.html
sidebarDepth: 2
lang: en-US
---

# Quick Start to Develop OpenMQTTGateway

This document helps a new contributor. Maybe this is your first time with:

- Git
- PlatformIO
- ESP32 or ESP8266

It is normal to feel a bit lost at the beginning. This guide walks with you, step by step.

::: tip Already experienced?
If you only need the contribution rules and PR flow, jump to [Development contributions](./development.md). For community help or non-code support, see [Community participation](./community.md) and [Supporting the project](./support.md).
:::



In this document you will learn how to:

- Prepare the development environment on Windows or Linux.
- Build and flash the firmware (ESP32 / ESP8266 and other boards).
- Build and preview the documentation website in the docs folder.

At the end of each step you see a **Check**. Use it to confirm that the previous step is correct before you go to the next one. If a check fails, do not worry: read the step again, fix it, and try one more time.



## 1. Understand this project

OpenMQTTGateway is a firmware project. It runs on ESP32, ESP8266 and other boards. It connects many protocols (Bluetooth, RF, IR, LoRa, sensors) to MQTT.

The repository also contains the documentation website. The docs use VuePress and live in the docs folder.

Very simple view of the repository:

- main: C++ source code for the firmware.
- lib: reusable libraries.
- scripts: helper scripts for build, test, CI.
- docs: documentation website.

You will mainly work on:

- Firmware code: files under main and sometimes lib.
- Docs site: files under docs and docs/.vuepress.

**Check:** open the repository in a file explorer and confirm you see at least main, lib, scripts and docs folders. If not, read section 3 again.



## 2. Prepare your computer

You can use Windows or Linux. The steps are very similar. The examples use a terminal. On Windows you can use **Git Bash** or **WSL**; on Linux you can use your normal shell.

Not everybody needs to install the same tools. It depends on what you want to do:

- **Only firmware (with VS Code or terminal):** you mainly need Git, PlatformIO, VS Code (if you like GUI) and a bash shell.
- **Only documentation website:** you mainly need Git, Node.js and npm.
- **Both firmware and docs:** you need everything.

The next subsections explain this.

### 2.1 Common tools (for everyone)

Install these tools first. They are useful for **all** types of work:

- Git
- Visual Studio Code (recommended, but you can also use another editor)
- A bash shell
   - On Windows: Git Bash or WSL (Ubuntu is a good choice).

On Windows you can download:

- Git from the official site: https://git-scm.com/downloads
- Visual Studio Code from https://code.visualstudio.com/
- Git Bash comes with "Git for Windows".

On Linux you can install these with your package manager (for example apt on Ubuntu). For more help you can read the Git book: https://git-scm.com/book/en/v2/Getting-Started-Installing-Git.

**Check (common tools):** open a terminal (Git Bash, WSL, or Linux shell) and run:

```bash
git --version
```

If this command fails, install Git again and then close and reopen the terminal.

### 2.2 Tools for firmware development

For firmware builds you need **PlatformIO**. PlatformIO itself will install the right compiler and frameworks for your boards.

You have two main options:

1. Use **PlatformIO inside VS Code** (recommended for most users).
2. Use **PlatformIO CLI in the terminal** (good for advanced or CI usage).

#### 2.2.1 PlatformIO inside VS Code (recommended)

If you only plan to build firmware from VS Code, you usually do **not** need to install Python or PlatformIO CLI manually. The PlatformIO extension will download what it needs.

- Just install VS Code and the PlatformIO extension (see section 2.3).
- When you first open the project, PlatformIO will set up its own tools.

**Check:** after installing the extension and opening the project, the VS Code status bar shows something like "PlatformIO: Ready".

#### 2.2.2 PlatformIO command line (CLI) with Python (for terminal builds)

If you want to use the bash scripts (ci.sh, ci_build_firmware.sh, etc.) or build from a plain terminal, you need Python and PlatformIO CLI.

Install these extra tools:

- Python 3.10 or newer
- PlatformIO CLI (pip package)

PlatformIO is the main build system for this project. We will use it from VS Code and from the terminal. If you want to read more, the official docs are here: https://docs.platformio.org/.

On Windows you can download Python from: https://www.python.org/downloads/

On Linux you can install Python with your package manager.

Then install PlatformIO CLI globally with Python:

```bash
python3 -m pip install -U platformio
```

**Check (firmware tools):**

```bash
python3 --version
platformio --version
```

If you see version numbers, the install is good. If the platformio command is not found, check that Python added the scripts folder (for example ~/.local/bin on Linux) to your PATH, then open a new terminal and try again.

### 2.3 Tools for documentation website (Node.js and npm)
::: warning Note
This section is **only needed** if you want to **build or preview the site on your own computer**.  
:::

If you want to build or edit the **documentation website**, you also need Node.js and npm. You do **not** need these tools just to build firmware.

Install:

- Node.js 18 or newer
- npm (usually comes with Node.js)

On Windows you can get Node.js from: https://nodejs.org/en/download

On Linux you can install Node.js with your package manager or by following the docs: https://nodejs.org/en/download/package-manager.

**Check (docs tools):**

```bash
node --version
npm --version
```

If one of these commands fails, reinstall Node.js, close the terminal, and try again.

### 2.4 PlatformIO extension in Visual Studio Code
Now add PlatformIO inside VS Code. You can also see pictures of these steps in many ESP32 + PlatformIO tutorials, for example on the PlatformIO site: https://platformio.org/install/ide?install=vscode.

Steps (based on common PlatformIO + VS Code ESP32 guides, adapted for this project):

1. Open Visual Studio Code.
2. Click the Extensions icon on the left.
3. In the search box, type "PlatformIO IDE".
4. Click **Install** on the "PlatformIO IDE" extension.

When the install finishes, a new icon appears on the left sidebar. It looks like an ant or an alien head. This opens the PlatformIO view.

**Check:**

- In VS Code, you see the PlatformIO icon on the left.
- If you click it, PlatformIO opens without errors.

If you do not see the icon, restart VS Code and wait a few seconds.

**Friendly tip:** the first install can take several minutes because PlatformIO downloads tools for many boards. It happens only once. You can get a coffee while it works.



## 3. Get the source code

Now download this repository to your computer from GitHub.

1. Open a terminal (Git Bash, WSL or Linux shell).
2. Choose a parent folder where you want to keep your projects.
3. Run:

```bash
git clone https://github.com/1technophile/OpenMQTTGateway.git
cd OpenMQTTGateway
```

**Check:**

- Run:

   ```bash
   git status
   ```

- The output should say "On branch" and "nothing to commit".
- In your file explorer or with:

   ```bash
   ls
   ```

   you should see main, lib, scripts, docs and other files.

If the clone fails, check your internet connection or a possible proxy. Then try again. If you are new to Git, the GitHub "Hello World" guide can also help: https://docs.github.com/en/get-started/start-your-journey/hello-world.



## 4. Firmware development with Visual Studio Code and PlatformIO

This section explains how to build and flash the firmware using Visual Studio Code with the PlatformIO extension.

If you know the classic Arduino IDE, you can think of PlatformIO as a more powerful alternative. It manages many boards, libraries, and environments for you. The official introduction is here: https://docs.platformio.org/en/latest/what-is-platformio.html.

You do **not** create a new PlatformIO project. This repository is already a PlatformIO project. You just open it.

### 4.1 Open the project in VS Code

1. Start Visual Studio Code.
2. Click **File → Open Folder…**.
3. Select the OpenMQTTGateway folder you cloned in section 3.
4. Click **Open**.

VS Code will load the folder. PlatformIO will detect the platformio.ini file and start to prepare its toolchain (compiler, libraries, board support). You do not need to install these by hand.

**Check:**

- In the bottom status bar you see messages like "PlatformIO: Installing" and then "PlatformIO: Ready".
- You can also open a terminal inside VS Code (View → Terminal) and run:

   ```bash
   pio --version
   ```

   You should see the PlatformIO version.

If PlatformIO stays stuck on "Installing" for a long time, close VS Code, reopen it and wait again. Check that your internet connection is working.

### 4.2 Look at the PlatformIO project structure

PlatformIO projects usually have folders like (you will see similar names in many tutorials):

- src: main source code.
- lib: libraries.
- include: headers.
- platformio.ini: configuration for boards and environments.

In OpenMQTTGateway the structure is a bit different, because it is a large project:

- main: main C++ files for the firmware.
- lib: libraries used by the firmware.
- platformio.ini and environments.ini: list many PlatformIO environments.
- test: tests for PlatformIO unit test framework.

**Check:** open platformio.ini in VS Code and scroll. You should see several [env:...] sections. This means PlatformIO understands the project.

If you want to know more about this file, see the PlatformIO docs about configuration: https://docs.platformio.org/en/latest/projectconf/index.html.

### 4.3 Choose a PlatformIO environment

An "environment" in PlatformIO is a build target. It defines the board, framework and options. You can think of it as a named profile for a specific device.

To select the environment for your board:

1. Press Ctrl+Shift+P (on macOS use Cmd+Shift+P).
2. Type "PlatformIO: Switch Project Environment" and press Enter.
3. A list of environments appears (for example esp32dev-all-test, theengs-bridge and many others).
4. Choose an environment that matches your board. For a generic ESP32 DevKit you can start with **esp32dev-all-test**.

**Check:**

- Look at the bottom blue bar. You should see the selected environment name.

If you chose the wrong environment, repeat the steps and pick another one.

### 4.4 Build the firmware in VS Code

Now compile the firmware.

1. In VS Code, open the PlatformIO view (left sidebar icon).
2. In the PlatformIO toolbar (bottom of the window), click the **check** (✓) icon. This runs the "Build" task.

PlatformIO now compiles the code for the selected environment. This can take several minutes the first time.

**Check:**

- In the Terminal panel, the build ends with the word "SUCCESS".
- On disk, a file like:

   ```
   .pio/build/<your-env-name>/firmware.bin
   ```

   exists.

If the build fails, read the error at the end of the log. Take your time; errors are part of the normal developer life.

Common problems:

- Missing tools: check that platformio works from the terminal (section 2.2).
- Wrong environment: try with esp32dev-all-test first.

Fix the problem, then run the build again.

### 4.5 Upload the firmware to the board

After a successful build you can flash the firmware (upload the program to the board).

1. Connect your ESP board to the computer with a good USB cable.
2. In the PlatformIO toolbar click the **right arrow** (→) icon. This runs the "Upload" task.

PlatformIO selects a serial port automatically in many cases.

**Check:**

- The upload log ends with success, and no "Failed to connect" error.
- The board reboots after upload.

If the upload fails:

- Check the USB cable (some cables are power-only and do not carry data).
- On some boards you must press and hold the BOOT button during upload.
- Make sure no other program is using the same serial port (for example another serial monitor).
- You can set the serial port manually in platformio.ini with the upload_port option.

### 4.6 Open the Serial Monitor

The Serial Monitor lets you see log messages from the firmware.

1. In the PlatformIO toolbar click the **plug** icon. This opens the Serial Monitor.
2. Set the baud rate to 115200 if it is not already set.

**Check:**

- After reset, you see text from the board, for example boot messages and MQTT logs.

If the text is unreadable, check that the baud rate matches the value in platformio.ini (monitor_speed). In this project the default is usually 115200.

If you want to learn more about Serial Monitor problems, the PlatformIO docs have a short page: https://docs.platformio.org/en/latest/core/userguide/device/cmd_monitor.html.

### 4.7 Run PlatformIO tests (optional)

You can run unit tests for the project.

1. Open a terminal inside VS Code (View → Terminal).
2. In the project root, run:

```bash
pio test -e test
```

**Check:**

- The output ends with "SUCCESS". If tests fail, read which test failed and open the file under test to understand why.


## 5. Firmware development from the terminal with ci.sh

You can also work only from the terminal. This is useful if you prefer command line, use remote development, or want to reproduce the same steps as the CI (Continuous Integration, the automatic build that runs on GitHub).

All helper scripts live in the scripts folder. The main entry point is:

- scripts/ci.sh

This script calls other scripts:

- scripts/ci_build.sh
- scripts/ci_build_firmware.sh
- scripts/ci_prepare_artifacts.sh
- scripts/ci_site.sh
- scripts/ci_qa.sh
- scripts/ci_list-env.sh

The scripts are written for bash. On Windows always use **Git Bash** or **WSL** for them, not plain PowerShell or cmd.

If you are new to bash, a gentle introduction is here: https://ubuntu.com/tutorials/command-line-for-beginners.

### 5.1 Check that you are in the right place

1. Open a bash terminal.
2. Go to your clone:

```bash
cd path/to/OpenMQTTGateway
```

**Check:** run:

```bash
ls scripts main docs
```

You should see these folders listed. If not, you are in the wrong directory.

### 5.2 List available firmware environments

You can ask the helper scripts which PlatformIO environments exist.

```bash
./scripts/ci.sh list-env
```

**Check:**

- The command prints many names in columns (for example esp32dev-all-test, theengs-bridge, rfbridge-direct, and so on).

If the command fails with "command not found" or similar, confirm that bash is used and that the file has execute permission (on Linux you may need chmod +x scripts/ci.sh).

### 5.3 Quick build of one firmware (simple mode)

Use ci.sh with the build command to build one firmware from the terminal.

Example for a development build on esp32dev-all-test:

```bash
./scripts/ci.sh build esp32dev-all-test --mode dev --verbose
```

What this does:

- Checks that Python, PlatformIO and git are installed.
- Calls ci_build_firmware.sh to run platformio run -e esp32dev-all-test.
- Optionally prepares artifacts if you add deployment options.

Useful flags:

- --mode dev or --mode prod: choose development or production mode. For local work, **dev** is usually enough.
- --clean: remove old build files before building.
- --verbose: show more logs from PlatformIO.

**Check:**

- The command ends with a "Build Summary" block and "Status: SUCCESS".
- The folder .pio/build/esp32dev-all-test contains firmware.bin.

If the build fails, read the error line near the bottom. Fix missing tools (see section 2) or wrong environment name, then run the command again.

### 5.4 Use ci_build_firmware.sh directly (advanced control)

Sometimes you want to work closer to PlatformIO.

You can call scripts/ci_build.sh directly when you want more control or when you debug a CI issue:

```bash
./scripts/ci_build.sh esp32dev-all-test --dev-ota --clean --verbose
```

This script:

- Validates the environment name.
- Sets environment variables for the build (for example OMG_VERSION, OTA flags).
- Runs platformio run for the chosen environment.
- Verifies that firmware.bin and other files exist.

**Check:**

- At the end you see a "Build Summary" for that environment.
- The directory .pio/build/esp32dev-all-test contains .bin and .elf files.

If there are no artifacts, check that PlatformIO is installed and that the environment name matches one from ci_list-env.sh.

### 5.5 Prepare artifacts for sharing or flashing tools

After a successful build, you can create a clean set of files ready for release or for use with external flashing tools (for example esptool.py or web uploaders).

Use scripts/ci_prepare_artifacts.sh through ci_build.sh, or call it directly.

Example (after a build) with ci_build.sh wrapper:

```bash
./scripts/ci_build.sh esp32dev-all-test --mode prod --deploy-ready
```

Example direct call to ci_prepare_artifacts.sh:

```bash
./scripts/ci_prepare_artifacts.sh esp32dev-all-test --clean
```

This script:

- Copies firmware.bin, partitions.bin, bootloader.bin and other files from `.pio/build/<env>` into the output folder.
- Renames them with the environment name, for example esp32dev-all-test-firmware.bin.
- Creates archives for the libraries used by that environment.

**Check:**

- The folder generated/toDeploy (or your custom output) exists.
- It contains files like `<env>-firmware.bin` and one or more `*-libraries.tgz` archives.

If the script says the build directory is missing, run a firmware build first (section 5.3 or 5.4).

### 5.6 Run QA checks from the terminal

Before you send a pull request, it is good to check the code style.

Run:

```bash
./scripts/ci.sh qa --check
```

This uses scripts/ci_qa.sh to run clang-format checks on files under main by default.

**Check:**

- The summary says "Status: SUCCESS".

If there are formatting problems, you can auto-fix them:

```bash
./scripts/ci.sh qa --fix
```

After this, review the changes with git diff, then commit them.

### 5.7 Run the full pipeline (QA + firmware build + docs)

For a full local run, similar to CI, use:

```bash
./scripts/ci.sh all esp32dev-all-test --mode prod
```

This will:

- Run QA checks.
- Build the firmware.
- Build the documentation site in production mode (unless you add --no-site).

**Check:**

- At the end you see a "Complete Pipeline Summary" with "Status: SUCCESS".

If QA fails, fix style issues first. If the build fails, fix code or environment problems, then run again.

::: tip Next Steps
Once your code builds successfully, check the [Development contributions guide](./development.md) for:
- Code naming rules (ZgatewayXXX, ZsensorYYY, etc.)
- Code style and quality requirements
- How to open a pull request
- Contributing workflow
:::



## 6. Work on the documentation website (from the terminal)

The documentation website uses VuePress and Node.js. The site source is in the docs folder.

::: warning Note
This section is **only needed** if you want to **build or preview the site on your own computer**.  
If you make a **very small change** in a markdown (.md) file (for example fix a typo or add one sentence), you can:

- Edit the file in VS Code.
- Push your change and open a pull request.

In that case it is **not mandatory** to install Node.js, npm, or to run `./scripts/ci.sh site`. The CI on GitHub will build the site for you and show problems if there are any.

If you want to see your doc changes locally before you push (recommended for bigger edits), then follow the steps below.
:::

### 6.1 Install Node dependencies

From the project root:

```bash
cd OpenMQTTGateway   # only if you are not already inside
npm install
```

This command reads package.json and installs all Node modules needed for the docs.

**Check:**

- The command finishes without error.
- A node_modules folder exists in the project root.

If install is slow, your internet may be the reason. If it fails, try again later or clear the npm cache with:

```bash
npm cache clean --force
```

### 6.2 Build the docs site with ci.sh

If you want to check that everything still builds well after bigger doc changes, you can build the site locally.

The recommended way is:

```bash
./scripts/ci.sh site --mode prod --url-prefix / --version 1.8.0
```

Important options:

- `--mode dev` or `--mode prod`: development or production build (default: dev).
- `--url-prefix PATH`: base URL path for links, e.g. `/` for root or `/dev/` for dev (default: /dev/).
- `--version TAG`: version string written into docs/.vuepress/meta.json (default: edge).
- `--preview`: if added, starts a local HTTPS preview server after build.
- `--clean`: remove generated/site folder before build.
- `--insecure-curl`: allow curl to skip TLS verification if needed.

The script (scripts/ci_site.sh) will:

- Check that node, npm and openssl are available.
- Download a shared configuration file for the site.
- Create docs/.vuepress/meta.json with site info.
- Run npm run docs:build to build the site.

**Check:**

- The summary at the end says "Site Build Summary" and "Status: SUCCESS".
- The folder docs/.vuepress/dist exists and contains HTML files.

If openssl or node is missing, go back to section 2 and install them.

### 6.3 Preview the docs site locally

To preview the site in your browser:

```bash
./scripts/ci.sh site --mode dev --url-prefix /dev/ --version edge --preview
```

The script will start a local HTTPS server.

**Check:**

- The log prints a line similar to: "Preview server running at https://localhost:8443/dev/".
- Open that URL in your browser. You should see the OpenMQTTGateway documentation.

To stop the preview, go back to the terminal and press Ctrl+C.

### 6.4 Work on docs with plain npm commands

If you prefer not to use the ci.sh wrapper, you can work directly with npm and VuePress.

From the project root:

```bash
npm run docs:dev
```

This runs VuePress in development mode with hot reload. When you change a .md file under docs, the browser reloads.

For a production build:

```bash
npm run docs:build
```

**Check:**

- For docs:dev, the terminal prints a local URL like http://localhost:8080. Open it in the browser and see the docs.
- For docs:build, the folder docs/.vuepress/dist is created.

If docs:build fails with an OpenSSL error on new Node versions, set:

```bash
export NODE_OPTIONS="--openssl-legacy-provider"
```

then run the command again. The ci_site.sh script already does this for you.

### 6.5 Where to edit docs

- All documentation pages are markdown (.md) files under docs.
- This file itself is in docs/participate/quick_start.md.

To add or edit docs:

1. Open the docs folder in VS Code.
2. Change or create markdown files.
3. Run npm run docs:dev to check how the page looks.

**Check:** after a change and a page refresh, your new text appears on the site.



## 7. Typical workflows for contributors

This section gives some example "day to day" flows.

### 7.1 Quick firmware change with VS Code

1. Open the project folder in VS Code.
2. Choose the right environment (section 4.3).
3. Edit code in main or lib.
4. Build (section 4.4).
5. Upload (section 4.5).
6. Watch logs in Serial Monitor (section 4.6).

**Check:** your change has the expected effect on the real device.

### 7.2 Firmware change using terminal only

1. Open a bash terminal.
2. Go to the project root.
3. Build firmware with:

    ```bash
    ./scripts/ci.sh build <env> --mode dev --verbose
    ```

4. Flash firmware using PlatformIO CLI or another flash tool with the generated firmware file.

**Check:** the device boots with your new firmware and behaves as expected.

### 7.3 Change documentation and preview

1. Edit markdown files under docs.
2. In a terminal, run npm run docs:dev or ./scripts/ci.sh site --mode dev --preview.
3. Open the local URL in your browser.

**Check:** your text appears on the page and looks correct.



## 8. Troubleshooting

This list gives quick help for common problems.

- **Problem:** platformio command not found.
   - **Fix:** install it with python3 -m pip install -U platformio and open a new terminal.

- **Problem:** Build fails for all environments.
   - **Fix:** run ./scripts/ci.sh list-env to confirm you use a valid env. Check that you did not change platformio.ini or environments.ini in a wrong way.

- **Problem:** Build fails with missing libraries.
   - **Fix:** run a clean build (add --clean). If you work only with VS Code, press the trash icon or use the clean target from PlatformIO.

- **Problem:** Upload fails ("Failed to connect to ESP32").
   - **Fix:** choose the right serial port, check the cable, try holding BOOT during upload.

- **Problem:** Serial output is garbled.
   - **Fix:** set Serial Monitor baud to 115200 and confirm monitor_speed in platformio.ini is the same.

- **Problem:** npm install is very slow or fails.
   - **Fix:** check your internet connection. If needed run npm cache clean --force and try again.

- **Problem:** Site build fails with an OpenSSL error.
   - **Fix:** export NODE_OPTIONS="--openssl-legacy-provider" before running npm run docs:build, or use ./scripts/ci.sh site which already sets this.

If you still have problems after these steps, you can open an issue on the project GitHub page. Include:

- Your operating system (Windows or Linux, version).
- What you tried to do.
- The exact command you ran.
- The error message from the end of the log.

This information helps maintainers reproduce and fix the problem.



## 9. Glossary (simple words)

This glossary explains some words used in this guide.

- **Firmware**: the program that runs inside your ESP32 or ESP8266 board.
- **Repository (repo)**: the project folder stored on GitHub and on your computer.
- **Commit**: a saved set of changes in Git, with a message.
- **Branch**: a line of development in Git, like a separate copy where you work.
- **Pull Request (PR)**: a request to merge your branch into the main project on GitHub.
- **Environment (env)**: a PlatformIO configuration for a specific board and options.
- **CI (Continuous Integration)**: automatic scripts that build and test the project on every change.
- **Serial Monitor**: a window that shows text messages sent by your board over USB.
- **MQTT**: a lightweight network protocol used to send messages between devices.
- **VuePress**: a static site generator used to build this documentation.

If any word in this guide is not clear, you can search it on the web or ask in the project community. Many people had the same question before you.



## 10. External links and further reading

Here is a list of useful links related to tools used in this project:

- **OpenMQTTGateway project**: https://github.com/1technophile/OpenMQTTGateway
- **PlatformIO main site**: https://platformio.org/
- **PlatformIO installation for VS Code**: https://platformio.org/install/ide?install=vscode
- **PlatformIO documentation**: https://docs.platformio.org/
- **Visual Studio Code documentation**: https://code.visualstudio.com/docs
- **Git official site**: https://git-scm.com/
- **Pro Git book (free)**: https://git-scm.com/book/en/v2
- **GitHub getting started**: https://docs.github.com/en/get-started
- **Node.js documentation**: https://nodejs.org/en/docs
- **npm documentation**: https://docs.npmjs.com/
- **VuePress documentation** (v1, similar to what this project uses): https://v1.vuepress.vuejs.org/
- **MQTT introduction (HiveMQ)**: https://www.hivemq.com/mqtt-essentials/

You do **not** need to read all of them now. Keep this list as a bookmark and come back when you are curious or stuck.


## 11. Final words and friendly advice

Contributing to an open source project is a journey. The first steps (install tools, learn Git, learn PlatformIO) can feel slow. This is normal.

Some final tips:

- Change one thing at a time and test often.
- Keep your changes small; they are easier to review.
- Write clear commit messages.
- When something breaks, read the last 10–20 lines of the log first.
- Do not be afraid to ask questions; everyone started as a beginner.

You are now ready to work on both firmware and documentation for OpenMQTTGateway. Take your time, follow the checks after each step, and you will build confidence with the toolchain and the project. Step by step, you will become faster and more comfortable.


If you are new and want to "play" a bit before you change real logic, here are some safe exercises:

1. **Practice the build chain only.**  
   - Clone the repo, open it in VS Code, select `esp32dev-all-test`, build, and upload to your board.  
   - Check that you can see logs in the Serial Monitor.
2. **Make a tiny log change.**  
   - Find a `LOG` or `Serial` message in a gateway or sensor file under `main`.  
   - Change the text slightly (for example add a word), rebuild, upload, and verify you see the new message.
3. **Edit a small doc page.**  
   - Fix a typo or add one sentence of clarification in a markdown file under `docs`.  
   - Run `./scripts/ci.sh site --mode prod --preview` and confirm your change appears in the browser.
4. **Run QA locally.**  
   - Run `./scripts/ci.sh qa --check`. If there are issues, run `./scripts/ci.sh qa --fix` and see how files are changed.
5. **Open a small PR.**  
   - For example, only the doc change or a very small code improvement. This lets you learn the review process without much stress.

These small steps build confidence. After that you can move to bigger things: adding new sensors, improving MQTT payloads, or extending the web documentation.
