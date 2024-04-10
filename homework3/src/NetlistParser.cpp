#include "NetlistParser.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <regex>

NetlistParser::NetlistParser(const std::string& filePath) : filePath(filePath) {}

void NetlistParser::parse() {
    std::ifstream file(filePath);
    std::string line;
    while (getline(file, line)) {
        parseLine(line);
    }
            // After processing all operations
            
    for (auto& component : components) {
        // Check if the component is an output and hasn't been registered
        if (component.type == "output" && !component.isReg) {
            // Check if the wire component already exists
            std::string wireOperand = component.name + "wire";
            bool wireExists = std::any_of(components.begin(), components.end(), [&](const Component& comp) { return comp.name == wireOperand; });

            // If the wire component doesn't exist, add it to the components
            if (!wireExists) {
                Component wireComponent;
                wireComponent.type = "wire";
                wireComponent.name = wireOperand;
                wireComponent.width = component.width; // Set the width according to the component
                wireComponent.isSigned = component.isSigned; // Set the signedness according to the component
                wireComponent.isNew = true;
                components.push_back(wireComponent);

                #if defined(ENABLE_LOGGING)  
                std::cout << "Declared wire component: " << wireOperand << " with width " << component.width << (component.isSigned ? ", signed" : ", unsigned") << "\n";
                #endif 
            }

            // Add a registration operation for the output component
            Operation regOperation;
            regOperation.result = component.name;
            regOperation.operands.push_back(wireOperand);
            regOperation.opType = "REG";
            regOperation.width = component.width; // Set the width according to the component
            regOperation.isSigned = component.isSigned; // Set the signedness according to the component
            operations.push_back(regOperation);

            // Set the isReg flag for the output component
            component.isReg = true;
            // Replace other operations using the output as an operand with the new wire         
        }
 
    }
 
    // Replace operands in operations with the new wire if necessary
    for (auto& operation : operations) {
        for (auto& operand : operation.operands) {
            // Check if operand matches any output component that has been registered
            auto it = std::find_if(components.begin(), components.end(), [&](const Component& comp) {
                return comp.name == operand && comp.type == "output" && comp.isReg;
            });

            if (it != components.end()) {
                // If found, replace the operand with its corresponding wire
                std::string wireOperand = it->name + "wire";
                operand = wireOperand;
                std::cerr << "Multi-driven pin detected and resolved for " << it->name << ". Replaced with " << wireOperand << std::endl;
            }
        }
    }
    #if defined(ENABLE_LOGGING)  
    for (const auto& component : components) {
        std::cout << "Component: " << component.name << ", isReg: " << (component.isReg ? "true" : "false") << std::endl;
    }
    #endif       
}
bool isNumeric(const std::string& str);
bool isExactlyOne(const std::string& str);  
bool isOnlyWhitespace(const std::string& str);

#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <set>

void NetlistParser::modifyModuleName(std::string& moduleName) {
    // Check for leading digits and remove them
    moduleName.erase(0, moduleName.find_first_not_of("0123456789"));

    // Remove special characters and the character before them
    for (size_t i = 0; i < moduleName.size(); ++i) {
        if (!std::isalnum(moduleName[i])) {
            if (i > 0) {
                moduleName.erase(i - 1, 1); // Remove the character before the special character
                // Update 'i' as we removed a character
                --i;
            }
            moduleName.erase(i, 1); // Remove the special character
            // Decrement 'i' to recheck the character that moved into the current position
            --i;
        }
    }
}
// Trim leading and trailing whitespaces from a string
std::string trim(const std::string& str) {
    // Find the first non-whitespace character from the beginning
    auto start = std::find_if(str.begin(), str.end(), [](int c) {
        return !std::isspace(static_cast<unsigned char>(c));
    });

    // Find the first non-whitespace character from the end
    auto end = std::find_if(str.rbegin(), str.rend(), [](int c) {
        return !std::isspace(static_cast<unsigned char>(c));
    }).base();

    // Handle empty string case
    return (start < end ? std::string(start, end) : std::string());
}
/*void NetlistParser::parseIfOperations(const std::string& ifStatement, const std::string& condition) {
    // Here you can parse operations inside the if block and handle the condition
    // For example, you can create Operation objects, calculate width, signedness, etc.
    // For simplicity, I'm just printing the information for demonstration
    std::cout << "Parsing operations inside if block with condition: " << condition << std::endl;
    std::cout << "Statement inside if block: " << ifStatement << std::endl;

    // Parse the condition
    std::istringstream conditionStream(condition);
    std::string leftOperand, opSymbol, rightOperand;
    conditionStream >> leftOperand >> opSymbol >> rightOperand;

    // Handle the condition (you can implement logic based on your requirements)
    std::cout << "Condition parsed: " << leftOperand << " " << opSymbol << " " << rightOperand << std::endl;

    // Parse each line of operations inside the if block
    std::istringstream ifStatementStream(ifStatement);
    std::string line;
    while (getline(ifStatementStream, line)) {
        // Skip empty lines
        if (line.empty())
            continue;

        // Trim leading and trailing whitespaces from the line
        line = trim(line);

        // Check if the line is the closing curly brace '}', indicating the end of the if block
        if (line == "}") {
            break;
        }

        // Parse and handle the operation
        std::cout << "if-based operation: " << line <<std::endl;
        parseOperation(line);
    }
}*/


int NetlistParser::determineOperationWidth(const std::string& opType, const std::vector<std::string>& operands, const std::string& result) {
    int maxWidth = 0;

    if (opType == "COMP") {
        // Determine the maximum width among operands
        for (const auto& operand : operands) {
            if (isNumeric(operand) || isOnlyWhitespace(operand)) continue; // Skip constants or empty operands

            if (componentWidths.find(operand) != componentWidths.end()) {
                int width = componentWidths[operand];
                maxWidth = std::max(maxWidth, width);
            } else {
                std::cerr << "Error: Operand " << operand << " not found.\n";
                std::exit(EXIT_FAILURE); // Exit if an operand width cannot be determined.
            }
        }
    } else {
        // Determine the width based on the result
        if (componentWidths.find(result) != componentWidths.end()) {
            maxWidth = componentWidths[result];
        } else {
            std::cerr << "Error: Result " << result << " not found.\n";
            std::exit(EXIT_FAILURE); // Exit if the result width cannot be determined.
        }
    }

    return maxWidth;
}

bool isValidOperand(const std::string& operand, const std::vector<Component>& components) {
    // Check if the operand is a constant or whitespace
    if (isNumeric(operand) || isOnlyWhitespace(operand))
        return true;

    // Check if the operand is inside the components
    for (const auto& component : components) {
        if (operand == component.name)
            return true;
    }

    // Operand is neither constant, whitespace, nor inside components
    return false;
}
std::unordered_map<int, std::string> lastNodeNameByState;

void NetlistParser::parseOperation(const std::string& operationLine,const std::string& condition, int state, int prev_state) {
    std::string beforeEq = operationLine.substr(0, operationLine.find('='));
    std::string afterEq = operationLine.substr(operationLine.find('=') + 1);

    std::string result = beforeEq.substr(0, beforeEq.find(' '));
    std::istringstream afterEqStream(afterEq);

    std::string leftOperand, opSymbol, rightOperand, colon, mux_right;
    afterEqStream >> leftOperand >> opSymbol >> rightOperand >> colon >> mux_right;

    // Check if operands are valid
    if (!isValidOperand(leftOperand, components) || !isValidOperand(result, components) || !isValidOperand(rightOperand, components) || !isValidOperand(mux_right, components)) {
        std::cerr << "Error: Invalid operand in line: " << operationLine << "\n";
        std::exit(EXIT_FAILURE);
    }

    // Remove semicolon if present in rightOperand
    if (!rightOperand.empty() && rightOperand.back() == ';')
        rightOperand.pop_back();
    if (!mux_right.empty() && mux_right.back() == ';')
        mux_right.pop_back();

    Operation operation;
    operation.result = result;
    operation.operands.push_back(leftOperand);
    operation.operands.push_back(rightOperand);
    operation.symbol = opSymbol;

    // Utilize the isExactlyOne function to check operands
    if (opSymbol == "+" && (isExactlyOne(leftOperand) || isExactlyOne(rightOperand))) 
        operation.opType = "INC"; // Increment operation if one operand is exactly "1"
    else if (opSymbol == "-" && (isExactlyOne(leftOperand) || isExactlyOne(rightOperand))) 
        operation.opType = "DEC"; // Decrement operation if one operand is exactly "1"
    else if (opSymbol == "+") 
        operation.opType = "ADD"; // Standard addition for other cases
    else if (opSymbol == "-") 
        operation.opType = "SUB"; // Standard subtraction for other cases
    else if (opSymbol == "*") 
        operation.opType = "MUL";        
    else if (opSymbol == "/") 
        operation.opType = "DIV";
    else if (opSymbol == ">>") 
        operation.opType = "SHR";        
    else if (opSymbol == "<<") 
        operation.opType = "SHL";        
    else if (opSymbol == "%") 
        operation.opType = "MOD"; 
    else if (opSymbol == ">" || opSymbol == "==" || opSymbol == "<") 
        operation.opType = "COMP";        
    else if (opSymbol == "?") {
        operation.opType = "MUX2x1";
        operation.operands.push_back(mux_right);
    }   
    else if (opSymbol == "") {
        // Find the corresponding component
        auto resultComponent = std::find_if(components.begin(), components.end(), [&](const Component& comp) { return comp.name == result; });

        // If the result component is found and is registered, assign operation type
        if (resultComponent != components.end() && !resultComponent->isReg){ 
            operation.opType = "REG";
            resultComponent->isReg = true;
        }    
    }    
    else {
        std::cerr << "Unsupported operation symbol: " << opSymbol << "\n";
        std::exit(EXIT_FAILURE); // Terminate the program for unsupported symbols
    }

    // Here you might want to call determineOperationWidth or assign a width directly
    operation.width = determineOperationWidth(operation.opType, operation.operands, operation.result);
    operation.isSigned = determineOperationSign(operation.opType, operation.operands, operation.result);
    operation.condition = condition; // Assign the current condition context to the operation
    operation.state = state;
    operation.prev_state = prev_state;
    std::string nodeName = operation.symbol + ":" + std::to_string(state); // Construct a unique node name/id
    operation.name = nodeName;
    operations.push_back(operation);

    #if defined(ENABLE_LOGGING)
    if (opSymbol == "?") 
        std::cout << "Parsed operation: " << result << " = " << leftOperand << " " << opSymbol << " " << rightOperand <<  " " << colon <<  " " << mux_right << "\t"; 
    else 
        std::cout << "Parsed operation: " << result << " = " << leftOperand << " " << opSymbol << " " << rightOperand << "\t";
    std::cout << " operation.width:"  << operation.width << "\n";
    std::cout << " operation.condition:"  << operation.condition << "\n";

    #endif

}


/*
int NetlistParser::determineOperationWidth(const std::string& opType, const std::vector<std::string>& operands,const std::string& result) {
    int maxWidth = 0;
    if (componentWidths.find(result) != componentWidths.end()) {
        maxWidth = componentWidths[result];
    }    
    for (const auto& operand : operands) {
        if (isNumeric(operand) || isOnlyWhitespace(operand)) continue; // Skip constants or empty operands

        std::cout << "Operand: " << operand << "\n"; // Debugging print   
        if (componentWidths.find(operand) != componentWidths.end()) {
            int width = componentWidths[operand];
            maxWidth = std::max(maxWidth, width);
        } else {
            std::cerr << "Error: Operand/Result " << operand << " not found.\n";
            std::exit(EXIT_FAILURE); // Exit if an operand width cannot be determined.
        }
    }
    return maxWidth;
}

*/
// Function to remove whitespace characters from a string
void removeWhitespace(std::string &str) {
    // Use std::remove_if with std::isspace to remove whitespace characters
    size_t resultPos = str.find_first_not_of(" \t");
    if (resultPos != std::string::npos) {
        str.erase(0, resultPos);
    }
}
bool NetlistParser::determineOperationSign(const std::string& opType, const std::vector<std::string>& operands,const std::string& result) {
     if (componentSignedness.find(result) != componentSignedness.end() && componentSignedness[result]) {
        return true; // Operation is signed if the result is signed
    }   
    for (const auto& operand : operands) {
        // Check if the operand exists in the componentSignedness map
        if (isNumeric(operand) || isOnlyWhitespace(operand)) continue; // Skip constants or empty operands
        if (componentSignedness.find(operand) != componentSignedness.end()) {
            // If any operand is signed, the operation is considered signed
            if (componentSignedness[operand]) {
                return true;
            }
        } else {
            std::cerr << "Error: Operand/Result " << operand << " not found.\n";
            std::exit(EXIT_FAILURE); // Exit if an operand's signedness cannot be determined.
        }
    }
    // Return false if none of the operands are signed
    return false;
}
int state = 0;
int prev_state = -1;
void NetlistParser::parseLine(const std::string& line) {
    size_t commentPos = line.find("//");

    //std::istringstream stream(line);
    // Extract the part of the line before the comment (if any)
    std::string cleanedLine = (commentPos != std::string::npos) ? line.substr(0, commentPos) : line;

    // Continue parsing only if there's something left after removing the comment
    if (!cleanedLine.empty()) {
        std::istringstream stream(cleanedLine);
        std::string word;
        stream >> word;   

        if (word == "input" || word == "output" || word == "wire" || word == "variable") {
            std::string dataType;
            stream >> dataType;
            // Check data type validity
            if (!(dataType.substr(0, 3) == "Int" && dataType.length() > 3 && isdigit(dataType[3])) &&
                !(dataType.substr(0, 4) == "UInt" && dataType.length() > 4 && isdigit(dataType[4]))) {
                std::cerr << "Error: Data type must be 'Int' or 'UInt', found '" << dataType << "' in line: " << line << "\n";
                std::exit(EXIT_FAILURE);
            }      
            int width = std::stoi(dataType.substr(dataType.find_first_of("1234567890")));
        
            bool isSigned = dataType.substr(0, 3) == "Int";

            std::string restOfLine;
            getline(stream, restOfLine, '\n'); // Get the rest of the line after dataType
            std::istringstream restStream(restOfLine);
            std::string componentName;

            while (getline(restStream, componentName, ',')) {
                // Remove spaces from componentName
                componentName.erase(remove_if(componentName.begin(), componentName.end(), isspace), componentName.end());

                if (!componentName.empty()) {
                    Component component;
                    component.type = word;
                    if(component.type == "variable") component.isReg = true;
                    component.name = componentName;
                    component.width = width;
                    if(width == 1) component.isSigned = false;
                    else component.isSigned = isSigned;
                    components.push_back(component);
                    componentWidths[componentName] = width; // Update componentWidths map
                    componentSignedness[componentName] = isSigned; // Track signedness of components
                    #if defined(ENABLE_LOGGING)
                    std::cout << "Parsed " << word << ": " << componentName << " with width " << width << (component.isSigned ? ", signed" : ", unsigned") << "\n"; 
                    #endif
                }
            }
  
        } else {
            // Handling operation lines

            std::cout << cleanedLine << std::endl;
            removeWhitespace(cleanedLine); 
            if (cleanedLine.find("if") != std::string::npos) {
                // Correctly extract the condition when entering an 'if' block
                currentCondition = cleanedLine.substr(cleanedLine.find("(") + 1, cleanedLine.find(")") - cleanedLine.find("(") - 1);
                // Print the current condition for debugging
                std::cout << "Entering IF block, condition: " << currentCondition << std::endl;
                state++;
            }
            else if (cleanedLine.find("}") != std::string::npos) {
                // Correctly clear the condition after exiting an 'if' block
                std::cout << "Exiting IF block, clearing condition: " << currentCondition << std::endl;
                currentCondition.clear();
            }
            else{  
              size_t eqPos = cleanedLine.find('=');
              if (eqPos != std::string::npos) {
                parseOperation(cleanedLine,currentCondition,state++,prev_state++);  

              }
            }

            

        }
    }    
}



const std::vector<Component>& NetlistParser::getComponents() const {
    return components;
}

const std::vector<Operation>& NetlistParser::getOperations() const {
    return operations;
}

