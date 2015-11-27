import os
import glob
import re

import sys
import pdb

serverDir = os.path.realpath('..')
srcDir = serverDir + '/src'
testDir = serverDir + '/test'


def toClassNameCase(string):
    return re.sub("(^|_)(\w)", lambda match: match.group(2).upper(), string)


class InvalidCClassException(Exception):
    pass


class CClassMethod:
    def __init__(self, returnType, methodName, parameters):
        self.returnType = returnType
        self.methodName = methodName
        self.parameters = parameters

    def getParameterCount(self):
        if self.parameters == "":
            return 0
        return len(re.sub(r"\([^()]*\)", "", self.parameters).split(","))

    def getParameterNames(self):
        paramNames = []
        continueParam = None
        for param in self.parameters.split(","):
            # Merge value if it has unbalanced parens in it
            if continueParam is not None:
                param = continueParam + "," + param
                continueParam = None
            if param.count("(") != param.count(")"):
                continueParam = param
                continue

            # Find name of parameter
            if "(" in param:
                # Assume this is callback function
                paramNames.append(re.match(r"[^()]*\([^()]*\*\s*(\w+)\s*\)", param).group(1))
            else:
                # Match last word (\w)
                paramNames.append(re.match(r".*\W(\w+)\s*$", param).group(1))
        return paramNames


class CClass:
    def __init__(self, name):
        self.name = name
        self.__ctxClass = None
        self.__ctxName = None
        self.__methods = []

    def analyzeFields(self, structBody):
        seenCtxField = False

        # Iterate over fields in structure
        fieldsAsText = [s.strip() for s in structBody.split(";") if s.strip() != ""]
        #pdb.set_trace()
        for fieldAsText in fieldsAsText:
            # Match context object
            match = CTX_PATTERN.match(fieldAsText)
            if match:
                if seenCtxField:
                    raise InvalidCClassException("Class " + self.name + " has multiple fields matching CTX_PATTERN")
                seenCtxField = True
                self.__ctxClass = match.group(1)
                self.__ctxName = match.group(2)
                continue

            # Match method
            match = METHOD_PATTERN.match(fieldAsText)
            if match:
                returnType = match.group(1)
                methodName = match.group(2)
                parameters = match.group(3).strip()
                firstParam = SELF_PARAMETER_PATTERN.match(parameters)
                if not firstParam:
                    raise InvalidCClassException("Couldn't match self parameter in method " + methodName + " in " + self.name)
                if firstParam.group(1) != self.name:
                    raise InvalidCClassException("Unmatched self param type " + firstParam.group(1) + ", expected " + self.name)

                # Remove first (self) param from parameters
                parameters = parameters[firstParam.end():].strip()
                self.__methods.append(CClassMethod(returnType, methodName, parameters))
                continue

            raise InvalidCClassException("Unmatched field \"" + fieldAsText + "\" in " + self.name)

    def getMockName(self):
        return "Mock" + toClassNameCase(self.name)

    def writeDeclarationToFile(self, f):
        mockName = self.getMockName()
        # Constructor and getStruct()
        f.write(
            "class " + mockName + " {\n" +
            "public:\n" +
            "    " + mockName + "();\n" +
            "    struct " + self.name + " *getStruct();\n" +
            "\n"
        )
        # MOCK_METHOD(...) 's
        for method in self.__methods:
            f.write(
                "    MOCK_METHOD" + str(method.getParameterCount()) +
                "(" + method.methodName + ", " + method.returnType + "(" + method.parameters + "));\n"
            );
        # Private c2cpp_* functions declaration
        f.write(
            "\n"
            "private:\n"
        )
        for method in self.__methods:
            parameters = self.name + " *self"
            if method.parameters:
                parameters += ", " + method.parameters
            f.write(
                "    static " + method.returnType + " c2cpp_" + method.methodName + "(" + parameters + ");\n"
            );
        # End class
        f.write(
            self.name + " mockedObject;\n" +
            "};\n"
        )

    def writeImplementationToFile(self, f):
        mockName = self.getMockName()
        # Constructor
        f.write(
            "" + mockName + "::" + mockName + "() {\n" +
            "    mockedObject." + self.__ctxName + " = (" + self.__ctxClass + " *) this;\n"
        )
        # Set C struct methods in constructor
        for method in self.__methods:
            f.write(
                "    mockedObject." + method.methodName + " = c2cpp_" + method.methodName + ";\n"
            );
        # End constructor and getStruct() method
        f.write(
            "}\n" +
            "\n" +
            # getStruct() method
            "struct " + self.name + " *" + mockName + "::getStruct()\n" +
            "{\n" +
            "    return &mockedObject;\n" +
            "}\n" +
            "\n"
        )
        # c2cpp_* methods implementation
        for method in self.__methods:
            if method.returnType != "void":
                returnStatementIfNeeded = "return "
            else:
                returnStatementIfNeeded = ""

            if method.parameters != "":
                parameters = ", " + method.parameters
                parameterNames = ", ".join(method.getParameterNames())
            else:
                parameters = ""
                parameterNames = ""

            f.write(
                method.returnType + " " + mockName + "::c2cpp_" + method.methodName +
                "(" + self.name + " *self" + parameters + ") {\n" +
                "    " + mockName + " *thiz = (" + mockName + " *) self->" + self.__ctxName + ";\n" +
                "    " + returnStatementIfNeeded + "thiz->" + method.methodName + 
                "(" + parameterNames + ");\n" +
                "}\n"
            );

# "typedef struct cls { ctx field and methods } cls;"
# "typedef struct 1 { 2 } 3;"
TYPEDEF_STRUCT_PATTERN = re.compile(r"""typedef\s*struct\s*(\w*)\s*{([^\{\}]*)}\s*(\w+)\s*;""")

# "cls_ctx *ctx"
# "1 *2;"
CTX_PATTERN = re.compile(r"""^(\w*)\s*\*\s*(\w*)\s*$""")

# "return_type (*method)(parameters)"
# 1 (*2)(3)"
METHOD_PATTERN = re.compile(r"""^(\w+)\s*\(\s*\*\s*(\w+)\s*\)\s*\((.*)\)$""")

# "struct cls *self,"
# "struct 1 *2,"
SELF_PARAMETER_PATTERN = re.compile(r"""^struct\s*(\w+)\s*\*(\w*)(,|$)""")

COMMENTS_PATTERN = re.compile("""(/\*.*?\*/|//[^\r\n]*)""", re.DOTALL)

GENERATED_FILE_WARNING = (
    "\n"
    r"/*         ^                                     */" "\n"        
    r"/*        / \           AUTO GENERATED FILE      */" "\n"        
    r"/*       /   \          DO NOT EDIT MANUALLY     */" "\n"        
    r"/*      /  |  \                                  */" "\n"        
    r"/*     /   |   \                                 */" "\n"        
    r"/*    /         \       Run generatemocks.py     */" "\n"        
    r"/*   /     o     \     to regenerate this file   */" "\n"        
    r"/*  /_____________\                              */" "\n"        
    "\n"
)

if __name__ == "__main__":
    for srcHeader in glob.glob(srcDir + '/*.h'):
        with open(srcHeader, 'r') as f:
            text = f.read()
        text = COMMENTS_PATTERN.sub("", text)
        for structMatch in TYPEDEF_STRUCT_PATTERN.finditer(text):
            c = CClass(structMatch.group(1))
            c.analyzeFields(structMatch.group(2))
            mockName = c.getMockName();
            guardname = "MOCK_" + c.name.upper() + "_H"
            with open(testDir + "/" + mockName + ".h", "w") as f:
                f.write(
                    "#ifndef " + guardname + "\n" +
                    "#define " + guardname + "\n" +
                    "extern \"C\" {\n" +
                    "#include \"../src/" + os.path.basename(srcHeader) + "\"\n" +
                    "}\n"
                    "#include <gmock/gmock.h>\n" +
                    GENERATED_FILE_WARNING
                )
                c.writeDeclarationToFile(f)
                f.write(
                    "#endif\n"
                )
            with open(testDir + "/" + mockName + ".cpp", "w") as f:
                f.write(
                    "#include \"" + mockName + ".h\"\n" +
                    GENERATED_FILE_WARNING
                )
                c.writeImplementationToFile(f)
