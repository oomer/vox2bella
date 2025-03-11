#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <map>
#include <filesystem>

#include "bella_sdk/bella_scene.h"
#include "dl_core/dl_main.inl"
using namespace dl;
using namespace bella_sdk;

/*
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
The children bytes value, tells the program how many bytes to skip, if the program is not concerned with the data contained in the child chunks.
*/

bool has_palette = false;

std::string initializeGlobalLicense();
std::string initializeGlobalThirdPartyLicences();

struct VoxHeader 
{
    char magic[4]; // "VOX "
    uint32_t version;
};

struct ChunkHeader 
{
    char id[4];
    uint32_t contentBytes;
    uint32_t childrenBytes;
};

struct Material {
    int32_t materialId;
    std::map<std::string, std::variant<std::string, float, bool>> properties;
};

void printMaterialProperties(const Material& matl) {
    if (matl.properties.count("_type") > 0 ) 
    {
        std::cout << "_type: " << std::get<std::string>(matl.properties.at("_type")) << std::endl;
    }

    if (matl.properties.count("_weight") > 0 )
    {
        std::cout << "_weight: " << std::get<float>(matl.properties.at("_weight")) << std::endl;
    }

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

//std::vector<uint8_t> voxelPalette;

// recursively calleable function to traverse children chunks
void readChunk( std::ifstream& file, 
                unsigned int (&palette)[256],
                std::vector<uint8_t> (&voxelPalette),
                Scene sceneWrite,
                Node voxel
              ) 
{
    ChunkHeader chunkHeader;
    // reinterpret_cast is a low level cast for known bit patterns
    file.read(reinterpret_cast<char*>(&chunkHeader), sizeof(ChunkHeader));

    std::string chunkId(chunkHeader.id, 4);
    //std::cout << "Chunk ID: " << chunkId << std::endl;
    //std::cout << "Content Bytes: " << chunkHeader.contentBytes << std::endl;
    //std::cout << "Children Bytes: " << chunkHeader.childrenBytes << std::endl;

    std::vector<uint8_t> content(chunkHeader.contentBytes); // array to store data
    file.read(reinterpret_cast<char*>(content.data()), chunkHeader.contentBytes);
    // Process chunk data based on chunkId
    /*if (chunkId == "PACK") 
    {
        uint32_t packSize = *reinterpret_cast<uint32_t*>(content.data());
        std::cout << packSize << std::endl;*/
    if (chunkId == "SIZE") 
    {
        uint32_t x = *reinterpret_cast<uint32_t*>(content.data());
        uint32_t y = *reinterpret_cast<uint32_t*>(content.data() + 4);
        uint32_t z = *reinterpret_cast<uint32_t*>(content.data() + 8);
        std::cout << "Size: " << x << "x" << y << "x" << z << std::endl;
    } else if (chunkId == "XYZI") 
    {
        uint32_t numVoxels = *reinterpret_cast<uint32_t*>(content.data());
        std::cout << "Number of Voxels: " << numVoxels << std::endl;

        // Start reading voxel data after the numVoxels value
        for (uint32_t i = 0; i < numVoxels; ++i) {
            /*uint32_t x = static_cast<uint32_t>(content[4 + (i * 4) + 0]);
            uint32_t y = static_cast<uint32_t>(content[4 + (i * 4) + 1]);
            uint32_t z = static_cast<uint32_t>(content[4 + (i * 4) + 2]);
            uint8_t colorIndex = content[4 + (i * 4) + 3];*/
            uint8_t x = content[4 + (i * 4) + 0];
            uint8_t y = content[4 + (i * 4) + 1];
            uint8_t z = content[4 + (i * 4) + 2];
            uint8_t colorIndex = content[4 + (i * 4) + 3]; 
            
            String voxXformName = String("voxXform") + String(i); // Create the node name
            auto xform  = sceneWrite.createNode("xform",voxXformName,voxXformName);
            xform.parentTo(sceneWrite.world());
            voxel.parentTo(xform);
            xform["steps"][0]["xform"] = Mat4 { 1, 0, 0, 0, 
                                                0, 1, 0, 0, 
                                                0, 0, 1, 0, 
                                                static_cast<double>(x*1), 
                                                static_cast<double>(y*1), 
                                                static_cast<double>(z*1), 1};
            voxelPalette.push_back(colorIndex);
        }

    // RGBA is a fixed size 256 color palette
    /*} else if (chunkId == "RGBA") 
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
    } else if (chunkId == "rCAM")
    {
        std::cout << "rCAM" << std::endl;
    } else if (chunkId == "PACK")
    {
        std::cout << "PACK" << std::endl;
    } else if (chunkId == "rOBJ")
    {
        std::cout << "rOBJ" << std::endl;
    } else if (chunkId == "nTRN")
    {
        std::cout << "nTRN" << std::endl;
    } else if (chunkId == "nGRP")
    {
        std::cout << "nGRP" << std::endl;
    } else if (chunkId == "nSHP")
    {
        std::cout << "nSHP" << std::endl;
    } else if (chunkId == "MATL")
    {
        Material material;
        size_t offset = 0;

        material.materialId = *reinterpret_cast<const int32_t*>(content.data() + offset);
        std::cout << "MaterialID:" << material.materialId << std::endl;
        offset += 4;
        uint32_t junk = *reinterpret_cast<uint32_t*>(content.data() + offset);
        //std::cout << "junk:" << junk << std::endl;
        offset += 4;

        // Hex Dump of the MATL Chunk
        /*std::cout << "MATL Chunk Hex Dump:" << std::endl;
        for (size_t i = 0; i < content.size(); ++i) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(content[i]) << " ";
            if ((i + 1) % 16 == 0) {
                std::cout << std::endl;
            }
        }
        std::cout << std::dec << std::endl;*/

        // multiple possible key-pairs stored as length, string, length, value
        while (offset < content.size()) {
            
            uint32_t keyLength = *reinterpret_cast<uint32_t*>(content.data() + offset);
            offset += 4;

            std::string key(reinterpret_cast<char*>(content.data() + offset), keyLength);
            std::cout << "Key: " << key << std::endl; // Debug: Print the key
            offset += keyLength;
            
            uint32_t valueLength = *reinterpret_cast<uint32_t*>(content.data() + offset);
            //std::cout << valueLength << std::endl;
            offset += 4;
            
            std::string valueStr(reinterpret_cast<char*>(content.data() + offset), valueLength);
            std::cout << "Value: " << valueStr << std::endl; // Debug: Print the key
            offset += valueLength;
            
            material.properties[key] = valueStr; // Store other values as strings

            /*if(key=="_rough" || key=="_weight" || key=="_spec" || key=="_ior" || key=="_att" || key=="_flux") {
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
        



    } else if (chunkId == "MATT")
    {
        std::cout << "MATT" << std::endl;
    } else if (chunkId == "LAYR")
    {
        std::cout << "LAYR" << std::endl;
    } else if (chunkId == "IMAP")
    {
        std::cout << "IMAP" << std::endl;
    } else if (chunkId == "NOTE")
    {
        std::cout << "NOTE" << std::endl;
    }
    //... handle other chunk ID's.

    //parse child chunks.
    std::streampos currentPosition = file.tellg();
    std::streampos childrenEnd = currentPosition + static_cast<std::streamoff>(chunkHeader.childrenBytes);
    while(file.tellg() < childrenEnd){
        readChunk(file, palette, voxelPalette, sceneWrite, voxel); // chunks can contain chunks
    }
}

// The SceneObserver is called when various events occur in the scene. There is also a NodeObserver
// to capture events from a single node, which can be useful when writing a user interface.
//  
struct Observer : public SceneObserver
{   
    bool inEventGroup = false;
        
    void onNodeAdded( Node node ) override
    {   
        logInfo("%sNode added: %s", inEventGroup ? "  " : "", node.name().buf());
    }
    void onNodeRemoved( Node node ) override
    {
        logInfo("%sNode removed: %s", inEventGroup ? "  " : "", node.name().buf());
    }
    void onInputChanged( Input input ) override
    {
        logInfo("%sInput changed: %s", inEventGroup ? "  " : "", input.path().buf());
    }
    void onInputConnected( Input input ) override
    {
        logInfo("%sInput connected: %s", inEventGroup ? "  " : "", input.path().buf());
    }
    void onBeginEventGroup() override
    {
        inEventGroup = true;
        logInfo("Event group begin.");
    }
    void onEndEventGroup() override
    {
        inEventGroup = false;
        logInfo("Event group end.");
    }
};

//#include "dl_core/dl_main.inl"
int DL_main(Args& args)
{

    std::string filePath;

    args.add("vi",  "voxin", "",   "Input .vox file");
    args.add("tp",  "thirdparty",   "",   "prints third party licenses");
    args.add("li",  "licenseinfo",   "",   "prints license info");

    if (args.versionReqested())
    {
        printf("%s", bellaSdkVersion().toString().buf());
        return 0;
    }

    if (args.helpRequested())
    {
        printf("%s", args.help("vox2bella", fs::exePath(), "Hello\n").buf());
        return 0;
    }
    if (args.have("--licenseinfo"))
    {
        std::cout << initializeGlobalLicense() << std::endl;
        return 0;
    }
 

    if (args.have("--thirdparty"))
    {
        std::cout << initializeGlobalThirdPartyLicences() << std::endl;
        return 0;
    }

    if (args.have("--voxin"))
    {
        filePath = args.value("--voxin").buf();
    } else 
    {
        std::cout << "Mandatory -vi .vox input missing" << std::endl;
        return 1; 
    }


    // Check for .vox extension
    if (filePath.length() < 5 || filePath.substr(filePath.length() - 4) != ".vox") {
        std::cerr << "Error: Input file must have a .vox extension." << filePath<<std::endl;
        return 1;
    }

    std::filesystem::path voxPath;

        // Check if the file exists (C++17 and later)
    if (!std::filesystem::exists(filePath)) {
        std::cerr << "Error: Input file does not exist." << std::endl;
        return 1;
    } else 
    {
        voxPath = std::filesystem::path(filePath);
    }

    Scene sceneWrite;
    sceneWrite.loadDefs();
        // Iterate through the vector
    std::vector<uint8_t> voxelPalette;
    /*unsigned int palette[256] = {
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
    };*/

    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error opening file." << std::endl;
        return 1;
    }

    VoxHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(VoxHeader));

    if (std::string(header.magic, 4) != "VOX ") {
        std::cerr << "Invalid file format." << std::endl;
        return 1;
    }
    
    // Set up a scene observer to see events occurring in the scene as we alter it.
    //
    //Observer observer;
    //sceneWrite.subscribe(&observer);

    //std::cout << "VOX Version: " << header.version << std::endl;

    // Create a few textures, connect them, and set some attr values.
    //
    auto beautyPass     = sceneWrite.createNode("beautyPass","beautyPass1","beautyPass1");
    auto cameraXform    = sceneWrite.createNode("xform","cameraXform1","cameraXform1");
    auto camera         = sceneWrite.createNode("camera","camera1","camera1");
    auto sensor         = sceneWrite.createNode("sensor","sensor1","sensor1");
    auto lens           = sceneWrite.createNode("thinLens","thinLens1","thinLens1");
    auto imageDome      = sceneWrite.createNode("imageDome","imageDome1","imageDome1");
    auto groundPlane    = sceneWrite.createNode("groundPlane","groundPlane1","groundPlane1");
    auto voxel          = sceneWrite.createNode("box","box1","box1");
    auto groundMat        = sceneWrite.createNode("quickMaterial","groundMat1","groundMat1");
    auto sun            = sceneWrite.createNode("sun","sun1","sun1");


    /*for( uint8_t i = 0; ;i++) 
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

    {
        Scene::EventScope es(sceneWrite);
        auto settings = sceneWrite.settings();
        auto world = sceneWrite.world();
        camera["resolution"]    = Vec2 {1920, 1080};
        camera["lens"]          = lens;
        camera["sensor"]        = sensor;
        camera.parentTo(cameraXform);
        cameraXform.parentTo(world);
        cameraXform["steps"][0]["xform"] = Mat4 {0.525768608156, -0.850627633385, 0, 0, -0.234464751651, -0.144921468924, -0.961261695938, 0, 0.817675761479, 0.505401223947, -0.275637355817, 0, -88.12259018466, -54.468125200218, 50.706001690932, 1};
        imageDome["ext"]            = ".jpg";
        imageDome["dir"]            = "./resources";
        imageDome["multiplier"]     = 6.0f;
        imageDome["file"]           = "DayEnvironmentHDRI019_1K-TONEMAPPED";

        groundPlane["elevation"]    = -.5f;
        groundPlane["material"]    = groundMat;
        /*sun["size"]    = 20.0f;
        sun["month"]    = "july";
        sun["rotation"]    = 50.0f;*/

        groundMat["type"] = "metal";
        groundMat["roughness"] = 22.0f;
        voxel["radius"]           = 0.33f;
        voxel["sizeX"]            = 0.99f;
        voxel["sizeY"]            = 0.99f;
        voxel["sizeZ"]            = 0.99f;

        settings["beautyPass"]  = beautyPass;
        settings["camera"]      = camera;
        settings["environment"] = imageDome;
        settings["iprScale"]    = 100.0f;
        settings["threads"]     = Input(0);
        settings["groundPlane"] = groundPlane;
        settings["iprNavigation"] = "maya";
        //settings["sun"] = sun;

    }


    while (file.peek() != EOF) {
        readChunk(file , palette, voxelPalette, sceneWrite, voxel);
    } 

    if (!has_palette)
    {
        for(int i=0; i<256; i++)
        {
            uint8_t r = (palette[i] >> 0) & 0xFF;
            uint8_t g = (palette[i] >> 8) & 0xFF;
            uint8_t b = (palette[i] >> 16) & 0xFF;
            uint8_t a = (palette[i] >> 24) & 0xFF;
            String nodeName = String("voxMat") + String(i); // Create the node name
            auto voxMat = sceneWrite.createNode("orenNayar",nodeName,nodeName);
            {
                Scene::EventScope es(sceneWrite);
                //dielectric["ior"] = 1.41f;
                //dielectric["roughness"] = 40.0f;
                //dielectric["depth"] = 33.0f;
                voxMat["reflectance"] = Rgba{ static_cast<double>(r)/255.0,
                                                    static_cast<double>(g)/255.0,
                                                    static_cast<double>(b)/255.0,
                                                    static_cast<double>(a)/255.0};
            }
            //dielectric
            /*{
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

    // Iterate through the vector
    for (int i = 0; i<voxelPalette.size(); i++) 
    {
        //std::cout << i << std::endl;
        auto xformNode = sceneWrite.findNode(String("voxXform")+String(i));
        auto matNode = sceneWrite.findNode(String("voxMat")+String(voxelPalette[i]));
        xformNode["material"] = matNode;
    }

    file.close();

    std::filesystem::path bszPath = voxPath.stem().string() + ".bsz";
    sceneWrite.write(String(bszPath.string().c_str()));

    return 0;
}

std::string initializeGlobalLicense() {
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
SOFTWARE.)"; }

std::string initializeGlobalThirdPartyLicences() {
return R"(
====

Bella SDK (Software Development Kit)

Copyright Diffuse Logic SCP, all rights reserved.

Permission is hereby granted to any person obtaining a copy of this software
(the "Software"), to use, copy, publish, distribute, sublicense, and/or sell
copies of the Software.

THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT EXPRESS OR IMPLIED WARRANTY. ALL
IMPLIED WARRANTIES OF FITNESS FOR ANY PARTICULAR PURPOSE AND OF MERCHANTABILITY
ARE HEREBY DISCLAIMED.

====

CppZMQ

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to
deal in the Software without restriction, including without limitation the
rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

====

libsodium


ISC License

Copyright (c) 2013-2025
Frank Denis <j at pureftpd dot org>

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

 =====

 libzmq is free software; you can redistribute it and/or modify it under the terms of the Mozilla Public License Version 2.0.)"; }
