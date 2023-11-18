import pytablereader as ptr
import pandas as pd
import os
import re
import configparser
conf = configparser.ConfigParser()

# Init the table with the columns
table_init = pd.DataFrame(columns=['Environment', 'uC', 'Hardware', 'Description', 'Modules', 'Platform',
                                   'Partitions', 'Libraries', 'Options'])
table = table_init

# Parse platformio.ini to retrieve boards information
conf.read('environments.ini')
for each_section in conf.sections():
    if ("env:" in each_section and "-test" not in each_section):
        env = each_section.replace("env:", "")
        uc = ""
        board = ""
        hardware = ""
        description = ""
        modules = ""
        platform = ""
        partitions = ""
        libraries = ""
        options = ""
        for (k, v) in conf.items(each_section):
            v = v.replace('{', '').replace('}', '').replace('$', '').replace(
                "env:", '').replace('\'', '').replace("-D", "")
            if (k == "board"):
                uc = v
            if (k == "platform"):
                platform = v
            if (k == "board_build.partitions"):
                partitions = v
            if (k == "lib_deps"):
                libraries = v
                libraries = libraries.replace(
                    "\ncom-esp.lib_deps\n", "").replace(
                    "\ncom-arduino.lib_deps\n", "").replace("libraries.", "")
            if (k == "build_flags"):
                options = v
                for o in options.split('\n'):
                    if ("gateway" in o or "sensor" in o or "actuator" in o):
                        if (modules != ""):
                            modules = modules + "\n"
                        modules = modules + o[1:o.rfind("=\"")]
                options = options.replace(
                    "com-esp.build_flags\n", "")
            if (k == "custom_description"):
                description = v
            if (k == "custom_hardware"):
                hardware = v
        table.loc[len(table.index)] = [env, uc, hardware, description, modules, platform,
                                       partitions, libraries, options]

# Sort rows per Environment name
table.sort_values(by=['Environment'], inplace=True,
                  key=lambda col: col.str.lower())

# Produce individual file
for ind in table.index:
    table_extract = table.iloc[ind]
    print(table_extract)
    file = open("docs/prerequisites/boards/" +
                table.iloc[ind]["Environment"] + ".md", 'w')
    table_extract = table_extract.rename_axis("Board index")
    table_md = table_extract.to_markdown()
    n = file.write(table_md)
    file.close()

# Produce list file
# Add link to the file from the environment and replace /n with ,
for ind in table.index:
    table['Environment'][ind] = "[" + table['Environment'][ind] + \
        "](../prerequisites/boards/" + table['Environment'][ind] + ")"

table = table.replace("\n", ", ", regex=True)
table = table.drop(["Partitions", "Hardware", "Platform", "Options","Modules"], axis=1)
table = table.reset_index(drop=True)
print(table)
# Convert to Markdown and save per Model_Id
table_md = table.to_markdown()
file = open("docs/prerequisites/board.md", 'a')
n = file.write("# Supported\n" + table_md)
file = open("docs/upload/web-install.md", 'a')
n = file.write(table_md)
file.close()
