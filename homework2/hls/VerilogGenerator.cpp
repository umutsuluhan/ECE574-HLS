#include "VerilogGenerator.h"

#include <fstream>
#include <sstream>
#include <algorithm> // For std::remove_if
#include <cctype> // For std::isspace

// Constructor definition
VerilogGenerator::VerilogGenerator(const std::vector<Component>& components, const std::vector<Operation>& operations)
    : components(components), operations(operations) {}

void VerilogGenerator::generateVerilog(const std::string& outputPath, const std::string& moduleName) {
    std::ofstream outFile(outputPath);
    std::stringstream moduleDecl, wires;

    // Start module declaration
    moduleDecl <<"`timescale 1ns / 1ps\n" << "module " << moduleName << "(\n";

    // Process components to generate module IOs and wires
    for (const auto& component : components) {
        std::string signModifier = component.isSigned ? " signed" : "";
        if (component.type == "input") {
            moduleDecl << "\tinput "<< signModifier << " [" << (component.width - 1) << ":0] " << component.name << ",\n";
        } else if (component.type == "output") {
            moduleDecl << "\toutput  "<< signModifier << " [" << (component.width - 1) << ":0] " << component.name << ",\n";
        } else if (component.type == "wire") {
            wires << "wire  "<< signModifier << " [" <<  (component.width - 1) << ":0] " << component.name << ";\n";
        }
    }

    // Remove the last comma and add closing parenthesis
    std::string moduleDeclStr = moduleDecl.str();
    moduleDeclStr = moduleDeclStr.substr(0, moduleDeclStr.rfind(',')) + "\n);";
    
    outFile << moduleDeclStr << "\n" << wires.str() << "\n";

    // Generate and write code for each operation
    for (const auto& operation : operations) {
        outFile << generateOperationCode(operation) << std::endl;
    }

    outFile << "endmodule\n";
}

std::string VerilogGenerator::generateComponentCode(const Component& component) const {
    std::stringstream ss;
    // Example: Generating Verilog for a wire
    if (component.type == "wire") {
        ss << "wire [" << (component.width - 1) << ":0] " << component.name << ";";
    }
    // Add more conditions for inputs/outputs if needed
    return ss.str();
}

std::string VerilogGenerator::generateOperationCode(const Operation& operation) const {
    std::stringstream ss;
    static std::unordered_map<std::string, int> opCounters;
    int count = ++opCounters[operation.opType];

    // Choose the submodule name based on signedness
    if (operation.opType == "ADD") { // Assuming "+" denotes ADD operation
        std::string moduleName = operation.isSigned ? "SADD" : "ADD";   
        // Example instantiation of an ADD datapath component
        ss << moduleName << " # (.DATAWIDTH("<< operation.width <<")) " << operation.opType << "_" << count <<
             " (.a("<< operation.operands[0]<<"),.b(" << operation.operands[1] 
           <<"),.sum("<< operation.result <<"));";
    }
    // Add instantiation code for more operation types as needed
    return ss.str();
}

