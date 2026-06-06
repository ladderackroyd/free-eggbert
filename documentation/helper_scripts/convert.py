import re
import os

INPUT_DIRECTORY_BASE = "../../include"
OUTPUT_DIRECTORY_BASE = "../diagrams"
FILENAME_IN = "../../include/button.hpp"
FILENAME_OUT = "test.puml"

PRIVATE = 0
PROTECTED = 1
PACKAGE_PRIVATE = 2
PUBLIC = 3

def getAccessModifierString(accessModifier):
    accessModifierString = "-"
    if (accessModifier == 3):
        accessModifierString = "+"
    elif (accessModifier == 2):
        accessModifierString = "~"
    elif (accessModifier == 1):
        accessModifierString = "#"
    else:
        accessModifierString = "-"
    return accessModifierString

def processFile(fileInBase, fileOutBase, fileNameBase):
    fileNameIn = fileInBase + "/" + fileNameBase
    fileNameOut = fileOutBase + "/" + fileNameBase.split(".hpp",1)[0] + ".puml"
    fileName = fileNameBase.split(".hpp",1)[0]
    with open(fileNameIn, "r", encoding="utf-8") as fileIn:
        with open(fileNameOut, "w", encoding="utf-8") as fileOut:
            current_access_modifier = PUBLIC
            current_indent = 0
            linebuilder = ""
            inComment = False
            notInClass = True
            lowestClassIndent = 0
            fileOut.write("@startuml " + fileName + "\n")
            for line in fileIn:
                #remove whitespace and semicolons
                line = re.sub(r"\/\/.*", "", line)
                line = re.sub(r"\t+", " ", line)
                line = re.sub(r"\/\*.*\*\/", "", line)
                line = line.strip()
                line = line.strip(";")
                #line omissions

                #ignore comments
                if line.startswith("//"):
                    continue
                #multi-line comment
                if not inComment and "/*" in line:
                    inComment = True
                    line = line.split("/*", 1)[0]
                if inComment and "*/" not in line:
                    continue
                if inComment and "*/" in line:
                    line = line.split("*/", 1)[1]
                    inComment = False
                #ignore compiler directives
                if line.startswith("#"):
                    continue
                #ignore empty lines
                if line == "":
                    continue

                #access modifiers
                if line.strip() == "public:":
                    current_access_modifier = PUBLIC
                    continue
                if line.strip() == "private:":
                    current_access_modifier = PRIVATE
                    continue
                if line.strip() == "protected:":
                    current_access_modifier = PROTECTED
                    continue
                
                #unmodified lines

                if line.startswith("class"):
                    if notInClass == True:
                        lowestClassIndent = current_indent
                        if current_indent > 0:
                            fileOut.write("}\n")
                    notInClass = False
                    current_access_modifier = PRIVATE
                    fileOut.write(line + "\n")
                    continue
                
                if line.startswith("typedef struct"):
                    if notInClass == True:
                        lowestClassIndent = current_indent
                        if current_indent > 0:
                            fileOut.write("}\n")
                    notInClass = False
                    current_access_modifier = PUBLIC
                    line = "class " + line.split("typedef struct", 1)[1] + "<<typedef struct>>"
                    fileOut.write(line + "\n")
                    continue
                
                if line.startswith("typedef"):
                    if notInClass == True:
                        lowestClassIndent = current_indent
                        if current_indent > 0:
                            fileOut.write("}\n")
                    notInClass = False
                    current_access_modifier = PUBLIC
                    line = "class " + line.split("typedef", 1)[1] + "<<typedef>>"
                    fileOut.write(line + "\n")
                    continue
                
                if line.startswith("struct"):
                    if notInClass == True:
                        lowestClassIndent = current_indent
                        if current_indent > 0:
                            fileOut.write("}\n")
                    notInClass = False
                    current_access_modifier = PUBLIC
                    line = "class " + line.split("struct", 1)[1] + "<<struct>>"
                    fileOut.write(line + "\n")
                    continue

                if line == "{":
                    current_indent = current_indent + 1
                    fileOut.write(line + "\n")
                    continue
                if line == "extern \"C\" {":
                    current_indent = current_indent + 1
                    fileOut.write("class " + fileName + " <<c_extern>> {\n")
                    current_access_modifier = PUBLIC
                    continue
                if line == "}":
                    current_indent = current_indent - 1
                    fileOut.write(line + "\n")
                    if (lowestClassIndent == current_indent):
                        notInClass = True
                        current_indent = current_indent + 1
                        fileOut.write("class " + fileName + " <<functions>> {\n")
                    continue

                #extern
                if line.startswith("extern "):
                    line = line.split("extern ", 1)[1]

                linebuilder = linebuilder + line
                if line.endswith(","):
                    continue
                
                if current_indent == 0:
                    current_indent = current_indent + 1
                    fileOut.write("class " + fileName + " <<functions>> {\n")
                    current_access_modifier = PUBLIC
                    continue
                #attributes and operations
                linebuilder = getAccessModifierString(current_access_modifier) + linebuilder.strip() + "\n"
                fileOut.write(linebuilder)
                linebuilder = ""
            
            while current_indent > 0:
                fileOut.write("}\n")
                current_indent = current_indent - 1

            fileOut.write("@enduml")

for root, subdirs, files in os.walk(INPUT_DIRECTORY_BASE):
    list_file_path = os.path.join(root, 'my-directory-list.txt')
    for filename in files:
        file_path = os.path.join(root, filename)
        file_in_path_base = file_path.split(INPUT_DIRECTORY_BASE, 1)[1]
        if file_in_path_base.startswith("\\"):
            file_in_path_base = file_in_path_base[1:]
        file_in_path_base = re.sub(r"\\", "/", file_in_path_base)
        processFile(INPUT_DIRECTORY_BASE, OUTPUT_DIRECTORY_BASE, file_in_path_base)