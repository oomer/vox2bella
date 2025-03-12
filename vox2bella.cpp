// vox2bella.cpp - A program to convert MagicaVoxel (.vox) files to Bella 3D scene (.bsz) files
// 
// This program reads MagicaVoxel files (which store voxel-based 3D models) and 
// converts them to Bella (a 3D rendering engine) scene files.

// Standard C++ library includes - these provide essential functionality
#include <iostream>     // For input/output operations (cout, cin, etc.)
#include <fstream>      // For file operations (reading/writing files)
#include <vector>       // For dynamic arrays (vectors)
#include <cstdint>      // For fixed-size integer types (uint8_t, uint32_t, etc.)
#include <map>          // For key-value pair data structures (maps)
#include <filesystem>   // For file system operations (directory handling, path manipulation)

// Bella SDK includes - external libraries for 3D rendering
#include "bella_sdk/bella_scene.h"  // For creating and manipulating 3D scenes in Bella
#include "dl_core/dl_main.inl"      // Core functionality from the Diffuse Logic engine

// Namespaces allow you to use symbols from a library without prefixing them
// For example, with these 'using' statements, you can write 'Scene' instead of 'bella_sdk::Scene'
using namespace dl;
using namespace bella_sdk;

/*
VOX File Format Structure Explanation:

Content Bytes:
These bytes contain the actual, immediate data associated with a specific chunk.
For example:
In a SIZE chunk, the content bytes hold the X, Y, and Z dimensions of the voxel grid.
In an XYZI chunk, they contain the voxel coordinates and color indices.
In an RGBA chunk, they hold the color palette information.
Essentially, the "content" is the data that the chunk is directly meant to store.

Children Bytes:
These bytes represent the size of any sub-chunks that are nested within the current chunk.
The .vox format allows for a hierarchical structure, where chunks can contain other chunks.
The "children" are these nested sub-chunks.
A "MAIN" chunk will have children bytes because it is the parent of most of the other chunks in the file.
If a chunk has no sub-chunks, its "children bytes" value will be zero.
The children bytes value tells the program how many bytes to skip if it's not concerned with child chunk data.
*/

// Global variable to track if we've found a color palette in the file
bool has_palette = false;

// Forward declarations of functions - tells the compiler that these functions exist 
// and will be defined later in the file
std::string initializeGlobalLicense();
std::string initializeGlobalThirdPartyLicences();

// Struct: A custom data type that groups multiple variables together
// This struct defines the header format for .vox files
struct VoxHeader 
{
    char magic[4];    // "VOX " - Identifier string that marks a valid .vox file
    uint32_t version; // Version number of the file format (uint32_t is a 32-bit unsigned integer)
};

// Struct for chunk headers in the .vox file
// A chunk is a section of data in the file with a specific purpose
struct ChunkHeader 
{
    char id[4];              // 4-character identifier for the chunk type (e.g., "SIZE", "XYZI")
    uint32_t contentBytes;   // Number of bytes of content in this chunk
    uint32_t childrenBytes;  // Number of bytes in child chunks
};

// Struct to store material properties from the .vox file
struct Material {
    int32_t materialId;  // ID number for this material
    // This map stores various properties a material can have
    // std::variant allows the value to be different types (string, float, or bool)
    std::map<std::string, std::variant<std::string, float, bool>> properties;
};

// Function to print material properties to the console
// The 'const' keyword means this function won't modify the material parameter
// The '&' means the parameter is passed by reference (avoiding a copy of the data)
void printMaterialProperties(const Material& matl) {
    // For each property, check if it exists and print it if it does
    // The 'count' method on a map returns how many items match the key (0 or 1)
    if (matl.properties.count("_type") > 0 ) 
    {
        // std::get<std::string> extracts the string value from the variant
        std::cout << "_type: " << std::get<std::string>(matl.properties.at("_type")) << std::endl;
    }

    if (matl.properties.count("_weight") > 0 )
    {
        // Here we're extracting a float value from the variant
        std::cout << "_weight: " << std::get<float>(matl.properties.at("_weight")) << std::endl;
    }

    // Similar checks for other material properties
    if (matl.properties.count("_rough") > 0 )
    {
        std::cout << "_rough: " << std::get<float>(matl.properties.at("_rough")) << std::endl;
    }

    if (matl.properties.count("_spec") > 0 )
    {
        std::cout << "_spec: " << std::get<float>(matl.properties.at("_spec")) << std::endl;
    }

    if (matl.properties.count("_ior") > 0 )
    {
        std::cout << "_ior: " << std::get<float>(matl.properties.at("_ior")) << std::endl;
    }

    if (matl.properties.count("_att") > 0 )
    {
        std::cout << "_att: " << std::get<float>(matl.properties.at("_att")) << std::endl;
    }

    if (matl.properties.count("_flux") > 0 )
    {
        std::cout << "_flux: " << std::get<float>(matl.properties.at("_flux")) << std::endl;
    }

    // Commented out properties - these were disabled in the original code
    /*if (matl.properties.count("_plastic") > 0 )
    {
        std::cout << "_plastic: " <<  std::endl;
    }*/

    /*if (matl.properties.count("_diffuse") > 0 )
    {
        std::cout << "_diffuse: " << std::endl;
    }

    if (matl.properties.count("_metal") > 0 )
    {
        std::cout << "_metal: " << std::endl;
    }

    if (matl.properties.count("_glass") > 0 )
    {
        std::cout << "_glass: " << std::endl;
    }

    if (matl.properties.count("_emit") > 0 )
    {
        std::cout << "_emit: " << std::endl;
    }*/
}

// Default color palette used if a .vox file doesn't provide its own
// This is an array of 256 unsigned integers, where each integer represents an RGBA color
// Format: 0xAARRGGBB (Alpha, Red, Green, Blue)
unsigned int palette[256] = {
    0x00000000, 0xffffffff, 0xffccffff, 0xff99ffff, 0xff66ffff, 0xff33ffff, 0xff00ffff, 0xffffccff, 0xffccccff, 0xff99ccff, 0xff66ccff, 0xff33ccff, 0xff00ccff, 0xffff99ff, 0xffcc99ff, 0xff9999ff,
    0xff6699ff, 0xff3399ff, 0xff0099ff, 0xffff66ff, 0xffcc66ff, 0xff9966ff, 0xff6666ff, 0xff3366ff, 0xff0066ff, 0xffff33ff, 0xffcc33ff, 0xff9933ff, 0xff6633ff, 0xff3333ff, 0xff0033ff, 0xffff00ff,
    0xffcc00ff, 0xff9900ff, 0xff6600ff, 0xff3300ff, 0xff0000ff, 0xffffffcc, 0xffccffcc, 0xff99ffcc, 0xff66ffcc, 0xff33ffcc, 0xff00ffcc, 0xffffcccc, 0xffcccccc, 0xff99cccc, 0xff66cccc, 0xff33cccc,
    0xff00cccc, 0xffff99cc, 0xffcc99cc, 0xff9999cc, 0xff6699cc, 0xff3399cc, 0xff0099cc, 0xffff66cc, 0xffcc66cc, 0xff9966cc, 0xff6666cc, 0xff3366cc, 0xff0066cc, 0xffff33cc, 0xffcc33cc, 0xff9933cc,
    0xff6633cc, 0xff3333cc, 0xff0033cc, 0xffff00cc, 0xffcc00cc, 0xff9900cc, 0xff6600cc, 0xff3300cc, 0xff0000cc, 0xffffff99, 0xffccff99, 0xff99ff99, 0xff66ff99, 0xff33ff99, 0xff00ff99, 0xffffcc99,
    0xffcccc99, 0xff99cc99, 0xff66cc99, 0xff33cc99, 0xff00cc99, 0xffff9999, 0xffcc9999, 0xff999999, 0xff669999, 0xff339999, 0xff009999, 0xffff6699, 0xffcc6699, 0xff996699, 0xff666699, 0xff336699,
    0xff006699, 0xffff3399, 0xffcc3399, 0xff993399, 0xff663399, 0xff333399, 0xff003399, 0xffff0099, 0xffcc0099, 0xff990099, 0xff660099, 0xff330099, 0xff000099, 0xffffff66, 0xffccff66, 0xff99ff66,
    0xff66ff66, 0xff33ff66, 0xff00ff66, 0xffffcc66, 0xffcccc66, 0xff99cc66, 0xff66cc66, 0xff33cc66, 0xff00cc66, 0xffff9966, 0xffcc9966, 0xff999966, 0xff669966, 0xff339966, 0xff009966, 0xffff6666,
    0xffcc6666, 0xff996666, 0xff666666, 0xff336666, 0xff006666, 0xffff3366, 0xffcc3366, 0xff993366, 0xff663366, 0xff333366, 0xff003366, 0xffff0066, 0xffcc0066, 0xff990066, 0xff660066, 0xff330066,
    0xff000066, 0xffffff33, 0xffccff33, 0xff99ff33, 0xff66ff33, 0xff33ff33, 0xff00ff33, 0xffffcc33, 0xffcccc33, 0xff99cc33, 0xff66cc33, 0xff33cc33, 0xff00cc33, 0xffff9933, 0xffcc9933, 0xff999933,
    0xff669933, 0xff339933, 0xff009933, 0xffff6633, 0xffcc6633, 0xff996633, 0xff666633, 0xff336633, 0xff006633, 0xffff3333, 0xffcc3333, 0xff993333, 0xff663333, 0xff333333, 0xff003333, 0xffff0033,
    0xffcc0033, 0xff990033, 0xff660033, 0xff330033, 0xff000033, 0xffffff00, 0xffccff00, 0xff99ff00, 0xff66ff00, 0xff33ff00, 0xff00ff00, 0xffffcc00, 0xffcccc00, 0xff99cc00, 0xff66cc00, 0xff33cc00,
    0xff00cc00, 0xffff9900, 0xffcc9900, 0xff999900, 0xff669900, 0xff339900, 0xff009900, 0xffff6600, 0xffcc6600, 0xff996600, 0xff666600, 0xff336600, 0xff006600, 0xffff3300, 0xffcc3300, 0xff993300,
    0xff663300, 0xff333300, 0xff003300, 0xffff0000, 0xffcc0000, 0xff990000, 0xff660000, 0xff330000, 0xff0000ee, 0xff0000dd, 0xff0000bb, 0xff0000aa, 0xff000088, 0xff000077, 0xff000055, 0xff000044,
    0xff000022, 0xff000011, 0xff00ee00, 0xff00dd00, 0xff00bb00, 0xff00aa00, 0xff008800, 0xff007700, 0xff005500, 0xff004400, 0xff002200, 0xff001100, 0xffee0000, 0xffdd0000, 0xffbb0000, 0xffaa0000,
    0xff880000, 0xff770000, 0xff550000, 0xff440000, 0xff220000, 0xff110000, 0xffeeeeee, 0xffdddddd, 0xffbbbbbb, 0xffaaaaaa, 0xff888888, 0xff777777, 0xff555555, 0xff444444, 0xff222222, 0xff111111
};

// Commented out: Alternative way to store the voxel palette
//std::vector<uint8_t> voxelPalette;

// Function to read a chunk from the .vox file
// Parameters:
// - file: Reference to the input file stream 
// - palette: Array of colors
// - voxelPalette: Vector to store color indices for each voxel
// - sceneWrite: The Bella scene being created
// - voxel: A node representing a voxel in the scene
//
// This function is recursively callable, meaning it can call itself to handle nested chunks
void readChunk( std::ifstream& file, 
                unsigned int (&palette)[256],
                std::vector<uint8_t> (&voxelPalette),
                Scene sceneWrite,
                Node voxel
              ) 
{
    // Read the chunk header from the file
    ChunkHeader chunkHeader;
    // reinterpret_cast is a C++ feature that converts one pointer type to another
    // It's used here to treat the ChunkHeader struct as a sequence of bytes for reading
    file.read(reinterpret_cast<char*>(&chunkHeader), sizeof(ChunkHeader));

    // Convert the 4-character ID to a string for easier comparison
    std::string chunkId(chunkHeader.id, 4);
    // Debug print statements (commented out)
    //std::cout << "Chunk ID: " << chunkId << std::endl;
    //std::cout << "Content Bytes: " << chunkHeader.contentBytes << std::endl;
    //std::cout << "Children Bytes: " << chunkHeader.childrenBytes << std::endl;

    // Create a vector to store the chunk content
    // 'content' is a dynamic array that will hold all the data in this chunk
    std::vector<uint8_t> content(chunkHeader.contentBytes);
    file.read(reinterpret_cast<char*>(content.data()), chunkHeader.contentBytes);
    
    // Process the chunk based on its ID
    // Different chunk types contain different data and need special handling
    
    /* Commented out PACK chunk handling
    if (chunkId == "PACK") 
    {
        uint32_t packSize = *reinterpret_cast<uint32_t*>(content.data());
        std::cout << packSize << std::endl;
    */
    
    if (chunkId == "SIZE") 
    {
        // SIZE chunk contains the dimensions of the voxel model (width, height, depth)
        // Read the XYZ size values from the content bytes
        uint32_t x = *reinterpret_cast<uint32_t*>(content.data());
        uint32_t y = *reinterpret_cast<uint32_t*>(content.data() + 4);
        uint32_t z = *reinterpret_cast<uint32_t*>(content.data() + 8);
        std::cout << "Size: " << x << "x" << y << "x" << z << std::endl;
    } 
    else if (chunkId == "XYZI") 
    {
        // XYZI chunk contains the voxel data - locations and colors of each voxel
        // First 4 bytes contain the number of voxels
        uint32_t numVoxels = *reinterpret_cast<uint32_t*>(content.data());
        std::cout << "Number of Voxels: " << numVoxels << std::endl;

        // Process each voxel
        // The voxel data is stored as a sequence of (x,y,z,colorIndex) values
        for (uint32_t i = 0; i < numVoxels; ++i) {
            // Commented out alternative approach
            /*uint32_t x = static_cast<uint32_t>(content[4 + (i * 4) + 0]);
            uint32_t y = static_cast<uint32_t>(content[4 + (i * 4) + 1]);
            uint32_t z = static_cast<uint32_t>(content[4 + (i * 4) + 2]);
            uint8_t colorIndex = content[4 + (i * 4) + 3];*/
            
            // Read voxel position (x,y,z) and color index
            // +4 offset is because the first 4 bytes contain numVoxels
            uint8_t x = content[4 + (i * 4) + 0];
            uint8_t y = content[4 + (i * 4) + 1];
            uint8_t z = content[4 + (i * 4) + 2];
            uint8_t colorIndex = content[4 + (i * 4) + 3]; 
            
            // Create a unique name for this voxel's transform node
            String voxXformName = String("voxXform") + String(i);
            // Create a transform node in the Bella scene
            auto xform = sceneWrite.createNode("xform", voxXformName, voxXformName);
            // Set this transform's parent to the world root
            xform.parentTo(sceneWrite.world());
            // Parent the voxel geometry to this transform
            voxel.parentTo(xform);
            // Set the transform matrix to position the voxel at (x,y,z)
            // This is a 4x4 transformation matrix - standard in 3D graphics
            xform["steps"][0]["xform"] = Mat4 { 1, 0, 0, 0, 
                                                0, 1, 0, 0, 
                                                0, 0, 1, 0, 
                                                static_cast<double>(x*1), 
                                                static_cast<double>(y*1), 
                                                static_cast<double>(z*1), 1};
            // Store the color index for this voxel
            voxelPalette.push_back(colorIndex);
        }

    /* Commented out RGBA chunk handling - would process color palette data
    } else if (chunkId == "RGBA") 
    {
        has_palette=true;
        for (int i = 0; i < 256; ++i) {
            palette[i] = *reinterpret_cast<uint32_t*>(content.data() + (i * 4));
            uint8_t r = (palette[i] >> 0) & 0xFF;
            uint8_t g = (palette[i] >> 8) & 0xFF;
            uint8_t b = (palette[i] >> 16) & 0xFF;
            uint8_t a = (palette[i] >> 24) & 0xFF;

            String nodeName = String("voxMat") + String(i); // Create the node name
            auto dielectric = sceneWrite.createNode("dielectric",nodeName,nodeName);
            {
                Scene::EventScope es(sceneWrite);
                dielectric["ior"] = 1.41f;
                dielectric["roughness"] = 40.0f;
                dielectric["depth"] = 33.0f;
                dielectric["transmittance"] = Rgba{ static_cast<double>(r)/255.0,
                                                    static_cast<double>(g)/255.0,
                                                    static_cast<double>(b)/255.0,
                                                    static_cast<double>(a)/255.0};
            }

            std::cout << "Color " << i << ": R=" << static_cast<int>(r)
                    << ", G=" << static_cast<int>(g)
                    << ", B=" << static_cast<int>(b)
                    << ", A=" << static_cast<int>(a) << std::endl; 
        }*/
    } 
    // Basic recognition of other chunk types - just printing their names
    else if (chunkId == "rCAM")
    {
        std::cout << "rCAM" << std::endl; // Camera settings chunk
    } 
    else if (chunkId == "PACK")
    {
        std::cout << "PACK" << std::endl; // Pack chunk (number of models)
    } 
    else if (chunkId == "rOBJ")
    {
        std::cout << "rOBJ" << std::endl; // Rendering settings
    } 
    else if (chunkId == "nTRN")
    {
        std::cout << "nTRN" << std::endl; // Node transform
    } 
    else if (chunkId == "nGRP")
    {
        std::cout << "nGRP" << std::endl; // Node group
    } 
    else if (chunkId == "nSHP")
    {
        std::cout << "nSHP" << std::endl; // Node shape
    } 
    else if (chunkId == "MATL")
    {
        // MATL chunk contains material definitions
        Material material;
        size_t offset = 0;

        // Read material ID (first 4 bytes)
        material.materialId = *reinterpret_cast<const int32_t*>(content.data() + offset);
        std::cout << "MaterialID:" << material.materialId << std::endl;
        offset += 4; // Move offset past the ID
        
        // Skip 4 bytes (possibly a dictionary size or other metadata)
        uint32_t junk = *reinterpret_cast<uint32_t*>(content.data() + offset);
        //std::cout << "junk:" << junk << std::endl;
        offset += 4;

        /* Commented out hex dump debugging code
        std::cout << "MATL Chunk Hex Dump:" << std::endl;
        for (size_t i = 0; i < content.size(); ++i) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(content[i]) << " ";
            if ((i + 1) % 16 == 0) {
                std::cout << std::endl;
            }
        }
        std::cout << std::dec << std::endl;*/

        // Parse material properties
        // The material data format is a series of key-value pairs:
        // [key length][key string][value length][value string]
        while (offset < content.size()) {
            // Read key length
            uint32_t keyLength = *reinterpret_cast<uint32_t*>(content.data() + offset);
            offset += 4;

            // Read key string
            std::string key(reinterpret_cast<char*>(content.data() + offset), keyLength);
            std::cout << "Key: " << key << std::endl; // Debug print
            offset += keyLength;
            
            // Read value length
            uint32_t valueLength = *reinterpret_cast<uint32_t*>(content.data() + offset);
            //std::cout << valueLength << std::endl;
            offset += 4;
            
            // Read value string
            std::string valueStr(reinterpret_cast<char*>(content.data() + offset), valueLength);
            std::cout << "Value: " << valueStr << std::endl; // Debug print
            offset += valueLength;
            
            // Store the property as a string
            material.properties[key] = valueStr;

            /* Commented out: Code to convert string values to floats for certain properties
            if(key=="_rough" || key=="_weight" || key=="_spec" || key=="_ior" || key=="_att" || key=="_flux") {
                try{
                    float valueFloat = std::stof(valueStr);
                    material.properties[key] = valueFloat;
                } catch (const std::invalid_argument& e){
                    std::cerr << "Error converting _rough to float: " << e.what() << std::endl;
                }
            } else {
                material.properties[key] = valueStr; // Store other values as strings
            }*/
        }

        //printMaterialProperties(material);
    } 
    // More chunk types that are identified but not fully processed
    else if (chunkId == "MATT")
    {
        std::cout << "MATT" << std::endl; // Legacy material chunk
    } 
    else if (chunkId == "LAYR")
    {
        std::cout << "LAYR" << std::endl; // Layer chunk
    } 
    else if (chunkId == "IMAP")
    {
        std::cout << "IMAP" << std::endl; // Index map chunk
    } 
    else if (chunkId == "NOTE")
    {
        std::cout << "NOTE" << std::endl; // Annotations
    }
    //... handle other chunk ID's.

    // Process child chunks if any
    // First get current position in the file
    std::streampos currentPosition = file.tellg();
    // Calculate where child chunks end
    std::streampos childrenEnd = currentPosition + static_cast<std::streamoff>(chunkHeader.childrenBytes);
    // Read child chunks until we reach the end of the children section
    while(file.tellg() < childrenEnd){
        // Recursively call readChunk to process each child chunk
        readChunk(file, palette, voxelPalette, sceneWrite, voxel);
    }
}

// Observer class for monitoring scene events
// This is a custom implementation of the SceneObserver interface from Bella SDK
// The SceneObserver is called when various events occur in the scene
struct Observer : public SceneObserver
{   
    bool inEventGroup = false; // Flag to track if we're in an event group
        
    // Override methods from SceneObserver to provide custom behavior
    
    // Called when a node is added to the scene
    void onNodeAdded( Node node ) override
    {   
        logInfo("%sNode added: %s", inEventGroup ? "  " : "", node.name().buf());
    }
    
    // Called when a node is removed from the scene
    void onNodeRemoved( Node node ) override
    {
        logInfo("%sNode removed: %s", inEventGroup ? "  " : "", node.name().buf());
    }
    
    // Called when an input value changes
    void onInputChanged( Input input ) override
    {
        logInfo("%sInput changed: %s", inEventGroup ? "  " : "", input.path().buf());
    }
    
    // Called when an input is connected to something
    void onInputConnected( Input input ) override
    {
        logInfo("%sInput connected: %s", inEventGroup ? "  " : "", input.path().buf());
    }
    
    // Called at the start of a group of related events
    void onBeginEventGroup() override
    {
        inEventGroup = true;
        logInfo("Event group begin.");
    }
    
    // Called at the end of a group of related events
    void onEndEventGroup() override
    {
        inEventGroup = false;
        logInfo("Event group end.");
    }
};

// Main function for the program
// This is where execution begins
// The Args object contains command-line arguments
int DL_main(Args& args)
{
    // Variable to store the input file path
    std::string filePath;

    // Define command-line arguments that the program accepts
    args.add("vi",  "voxin", "",   "Input .vox file");
    args.add("tp",  "thirdparty",   "",   "prints third party licenses");
    args.add("li",  "licenseinfo",   "",   "prints license info");

    // Handle special command-line requests
    
    // If --version was requested, print version and exit
    if (args.versionReqested())
    {
        printf("%s", bellaSdkVersion().toString().buf());
        return 0;
    }

    // If --help was requested, print help and exit
    if (args.helpRequested())
    {
        printf("%s", args.help("vox2bella", fs::exePath(), "Hello\n").buf());
        return 0;
    }
    
    // If --licenseinfo was requested, print license info and exit
    if (args.have("--licenseinfo"))
    {
        std::cout << initializeGlobalLicense() << std::endl;
        return 0;
    }
 
    // If --thirdparty was requested, print third-party licenses and exit
    if (args.have("--thirdparty"))
    {
        std::cout << initializeGlobalThirdPartyLicences() << std::endl;
        return 0;
    }

    // Get the input file path from command line arguments
    if (args.have("--voxin"))
    {
        filePath = args.value("--voxin").buf();
    } 
    else 
    {
        // If no input file was specified, print error and exit
        std::cout << "Mandatory -vi .vox input missing" << std::endl;
        return 1; 
    }

    // Validate the input file
    
    // Check that the file has a .vox extension
    if (filePath.length() < 5 || filePath.substr(filePath.length() - 4) != ".vox") {
        std::cerr << "Error: Input file must have a .vox extension." << filePath<<std::endl;
        return 1;
    }

    std::filesystem::path voxPath;

    // Check if the file exists
    if (!std::filesystem::exists(filePath)) {
        std::cerr << "Error: Input file does not exist." << std::endl;
        return 1;
    } 
    else 
    {
        voxPath = std::filesystem::path(filePath);
    }

    // Create a new Bella scene
    Scene sceneWrite;
    sceneWrite.loadDefs(); // Load scene definitions
    
    // Create a vector to store voxel color indices
    std::vector<uint8_t> voxelPalette;

    // Open the input file for binary reading
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error opening file." << std::endl;
        return 1;
    }

    // Read the VOX file header
    VoxHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(VoxHeader));

    // Validate that this is actually a VOX file by checking the magic number
    if (std::string(header.magic, 4) != "VOX ") {
        std::cerr << "Invalid file format." << std::endl;
        return 1;
    }
    
    // Commented out: Set up scene observer
    //Observer observer;
    //sceneWrite.subscribe(&observer);

    // Commented out: Print the VOX version
    //std::cout << "VOX Version: " << header.version << std::endl;

    // Create the basic scene elements in Bella
    // Each line creates a different type of node in the scene
    auto beautyPass     = sceneWrite.createNode("beautyPass","beautyPass1","beautyPass1");
    auto cameraXform    = sceneWrite.createNode("xform","cameraXform1","cameraXform1");
    auto camera         = sceneWrite.createNode("camera","camera1","camera1");
    auto sensor         = sceneWrite.createNode("sensor","sensor1","sensor1");
    auto lens           = sceneWrite.createNode("thinLens","thinLens1","thinLens1");
    auto imageDome      = sceneWrite.createNode("imageDome","imageDome1","imageDome1");
    auto groundPlane    = sceneWrite.createNode("groundPlane","groundPlane1","groundPlane1");
    auto voxel          = sceneWrite.createNode("box","box1","box1");
    auto groundMat      = sceneWrite.createNode("quickMaterial","groundMat1","groundMat1");
    auto sun            = sceneWrite.createNode("sun","sun1","sun1");

    /* Commented out: Loop to create material nodes
    for( uint8_t i = 0; ;i++) 
    {
        String nodeName = String("voxMat") + String(i); // Create the node name
        auto dielectric = sceneWrite.createNode("dielectric",nodeName,nodeName);
        {
            Scene::EventScope es(sceneWrite);
            dielectric["ior"] = 1.51f;
            dielectric["roughness"] = 22.0f;
            dielectric["depth"] = 44.0f;
        }
        if(i==255)
        {
            break;
        }
    }*/

    // Set up the scene with an EventScope 
    // EventScope groups multiple changes together for efficiency
    {
        Scene::EventScope es(sceneWrite);
        auto settings = sceneWrite.settings(); // Get scene settings
        auto world = sceneWrite.world();       // Get scene world root
        
        // Configure camera
        camera["resolution"]    = Vec2 {1920, 1080};  // Set resolution to 1080p
        camera["lens"]          = lens;               // Connect camera to lens
        camera["sensor"]        = sensor;             // Connect camera to sensor
        camera.parentTo(cameraXform);                 // Parent camera to its transform
        cameraXform.parentTo(world);                  // Parent camera transform to world
        
        // Position the camera with a transformation matrix
        cameraXform["steps"][0]["xform"] = Mat4 {0.525768608156, -0.850627633385, 0, 0, -0.234464751651, -0.144921468924, -0.961261695938, 0, 0.817675761479, 0.505401223947, -0.275637355817, 0, -88.12259018466, -54.468125200218, 50.706001690932, 1};
        
        // Configure environment (image-based lighting)
        imageDome["ext"]            = ".jpg";
        imageDome["dir"]            = "./resources";
        imageDome["multiplier"]     = 6.0f;
        imageDome["file"]           = "DayEnvironmentHDRI019_1K-TONEMAPPED";

        // Configure ground plane
        groundPlane["elevation"]    = -.5f;
        groundPlane["material"]     = groundMat;
        
        /* Commented out: Sun configuration
        sun["size"]    = 20.0f;
        sun["month"]    = "july";
        sun["rotation"]    = 50.0f;*/

        // Configure materials
        groundMat["type"] = "metal";
        groundMat["roughness"] = 22.0f;
        
        // Configure voxel box dimensions
        voxel["radius"]           = 0.33f;
        voxel["sizeX"]            = 0.99f;
        voxel["sizeY"]            = 0.99f;
        voxel["sizeZ"]            = 0.99f;

        // Set up scene settings
        settings["beautyPass"]  = beautyPass;
        settings["camera"]      = camera;
        settings["environment"] = imageDome;
        settings["iprScale"]    = 100.0f;
        settings["threads"]     = Input(0);  // Auto-detect thread count
        settings["groundPlane"] = groundPlane;
        settings["iprNavigation"] = "maya";  // Use Maya-like navigation in viewer
        //settings["sun"] = sun;
    }

    // Process all chunks in the VOX file
    // Loop until we reach the end of the file
    while (file.peek() != EOF) {
        readChunk(file, palette, voxelPalette, sceneWrite, voxel);
    } 

    // If the file didn't have a palette, create materials using the default palette
    if (!has_palette)
    {
        for(int i=0; i<256; i++)
        {
            // Extract RGBA components from the palette color
            // Bit shifting and masking extracts individual byte components
            uint8_t r = (palette[i] >> 0) & 0xFF;   // Red (lowest byte)
            uint8_t g = (palette[i] >> 8) & 0xFF;   // Green (second byte)
            uint8_t b = (palette[i] >> 16) & 0xFF;  // Blue (third byte)
            uint8_t a = (palette[i] >> 24) & 0xFF;  // Alpha (highest byte)
            
            // Create a unique material name
            String nodeName = String("voxMat") + String(i);
            // Create an Oren-Nayar material (diffuse material model)
            auto voxMat = sceneWrite.createNode("orenNayar", nodeName, nodeName);
            {
                Scene::EventScope es(sceneWrite);
                // Commented out: Alternative material settings
                //dielectric["ior"] = 1.41f;
                //dielectric["roughness"] = 40.0f;
                //dielectric["depth"] = 33.0f;
                
                // Set the material color (convert 0-255 values to 0.0-1.0 range)
                voxMat["reflectance"] = Rgba{ static_cast<double>(r)/255.0,
                                              static_cast<double>(g)/255.0,
                                              static_cast<double>(b)/255.0,
                                              static_cast<double>(a)/255.0};
            }
            
            /* Commented out: Alternative dielectric material creation
            {
                Scene::EventScope es(sceneWrite);
                dielectric["ior"] = 1.41f;
                dielectric["roughness"] = 40.0f;
                dielectric["depth"] = 33.0f;
                dielectric["transmittance"] = Rgba{ static_cast<double>(r)/255.0,
                                                    static_cast<double>(g)/255.0,
                                                    static_cast<double>(b)/255.0,
                                                    static_cast<double>(a)/255.0};
            }*/
        }
    }

    // Assign materials to voxels based on their color indices
    for (int i = 0; i < voxelPalette.size(); i++) 
    {
        // Find the transform node for this voxel
        auto xformNode = sceneWrite.findNode(String("voxXform") + String(i));
        // Find the material node for this voxel's color
        auto matNode = sceneWrite.findNode(String("voxMat") + String(voxelPalette[i]));
        // Assign the material to the voxel
        xformNode["material"] = matNode;
    }

    // Close the input file
    file.close();

    // Create the output file path by replacing .vox with .bsz
    std::filesystem::path bszPath = voxPath.stem().string() + ".bsz";
    // Write the Bella scene to the output file
    sceneWrite.write(String(bszPath.string().c_str()));

    // Return success
    return 0;
}

// Function that returns the license text for this program
std::string initializeGlobalLicense() 
{
    // R"(...)" is a C++ raw string literal - allows multi-line strings with preserved formatting
    return R"(
vox2bella

Copyright (c) 2025 Harvey Fong

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.)";
}

// Function that returns third-party license text
std::string initializeGlobalThirdPartyLicences() 
{
    return R"(
Bella SDK (Software Development Kit)

Copyright Diffuse Logic SCP, all rights reserved.

Permission is hereby granted to any person obtaining a copy of this software
(the "Software"), to use, copy, publish, distribute, sublicense, and/or sell
copies of the Software.

THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT EXPRESS OR IMPLIED WARRANTY. ALL
IMPLIED WARRANTIES OF FITNESS FOR ANY PARTICULAR PURPOSE AND OF MERCHANTABILITY
ARE HEREBY DISCLAIMED.)"; 
}
