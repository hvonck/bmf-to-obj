#include <string>
#include <vector>
#include <assert.h>

#ifdef __unix
#define fopen_s(pFile,filename,mode) ((*(pFile))=fopen((filename),  (mode)))==NULL
#endif

struct BMF_Geometry
{
  std::string           name = "";
  std::vector<uint32_t> indices;
  std::vector<float>    uvs;
  std::vector<float>    normals;
  std::vector<float>    positions;
  std::vector<float>    colors;
};

struct BMF_Object
{
  std::vector<BMF_Geometry> geometries;
};

void ExportToOBJ(const BMF_Object& object, std::string path)
{
  // NOTE: Colors are currently not supported in the OBJ file format.
  // If colors are required another exporter should be implemented.

  FILE* pFile = nullptr;
  fopen_s(&pFile, path.c_str(), "wb");

  auto Write = [&](std::string s) { fwrite(s.data(), s.size(), 1, pFile); };

  // Write out the elements.
  for (const BMF_Geometry& geometry : object.geometries)
  {
    // Write all positions.
    for (uint32_t i = 0; i < geometry.positions.size(); i += 3)
      Write("v " + std::to_string(geometry.positions[i + 0]) +
             " " + std::to_string(geometry.positions[i + 1]) +
             " " + std::to_string(geometry.positions[i + 2]) +
            "\n");

    // Write all uvs.
    for (uint32_t i = 0; i < geometry.uvs.size(); i += 2)
      Write("vt " + std::to_string(geometry.uvs[i + 0]) +
              " " + std::to_string(geometry.uvs[i + 1]) +
              "\n");

    // Write all normals.
    for (uint32_t i = 0; i < geometry.normals.size(); i += 3)
      Write("vn " + std::to_string(geometry.normals[i + 0]) +
              " " + std::to_string(geometry.normals[i + 1]) +
              " " + std::to_string(geometry.normals[i + 2]) +
              "\n");
  }

  uint32_t po = 1;
  uint32_t uo = 1;
  uint32_t no = 1;

  // Write out the models.
  for (const BMF_Geometry& geometry : object.geometries)
  {
    Write("o " + geometry.name + "\n");

    for (uint32_t i = 0; i < geometry.indices.size(); i += 3)
    {
      Write("f");

      for (uint32_t j = 0; j < 3; ++j)
      {
        uint32_t idx = i + j;
        uint32_t p = (idx < geometry.positions.size() ? idx + po : 0);
        uint32_t u = (idx < geometry.uvs.size()       ? idx + uo : 0);
        uint32_t n = (idx < geometry.normals.size()   ? idx + no : 0);
        Write(" " + (p ? std::to_string(p) : "") + "/" + (u ? std::to_string(u) : "") + "/" + (n ? std::to_string(n) : ""));
      }

      Write("\n");
    }

    po += geometry.positions.size() ? (uint32_t)geometry.indices.size() : 0;
    uo += geometry.uvs.size()       ? (uint32_t)geometry.indices.size() : 0;
    no += geometry.normals.size()   ? (uint32_t)geometry.indices.size() : 0;
  }

  fclose(pFile);
}

void Read(BMF_Object& object, std::string path)
{
  // Asserts.
  auto Assert   = [&](bool value, std::string msg) { assert(value && "Bad BMF File"); };
  auto AssertEq = [&](uint32_t a, uint32_t b)      { assert(a == b && "Not Equal"); };

  // Load in the file.
  FILE* pFile = nullptr;
  fopen_s(&pFile, path.c_str(), "rb");
  Assert(pFile, "Could not open file");

  // Constants.
  static constexpr uint32_t BMF_HAS_BMF       = 1112360496;
  static constexpr uint32_t BMF_GOT_BMF       = 1179468336;
  static constexpr uint32_t BMF_HAS_POSITIONS = 1399805488;
  static constexpr uint32_t BMF_GOT_POSITIONS = 1164924464;
  static constexpr uint32_t BMF_HAS_COLORS    = 1399800624;
  static constexpr uint32_t BMF_GOT_COLORS    = 1164919600;
  static constexpr uint32_t BMF_HAS_MESH      = 1399801648;
  static constexpr uint32_t BMF_GOT_MESH      = 1164920624;
  static constexpr uint32_t BMF_HAS_NAME      = 1298232368;
  static constexpr uint32_t BMF_GOT_NAME      = 1399801392;
  static constexpr uint32_t BMF_GOT_INDICES   = 1164920368;
  static constexpr uint32_t BMF_HAS_UVS       = 1399805232;
  static constexpr uint32_t BMF_GOT_UVS       = 1164924208;
  static constexpr uint32_t BMF_HAS_NORMALS   = 1399803440;
  static constexpr uint32_t BMF_GOT_NORMALS   = 1164922416;

  // Getter functions.
  auto GetData         = [&](const uint32_t& size, void* pData)    { AssertEq((uint32_t)fread(pData, size, 1, pFile), 1); };
  auto GetUint32       = [&]()->uint32_t                           { uint32_t v; GetData(sizeof(v), &v); return v; };
  auto GetFloat32      = [&]()->float                              { float    v; GetData(sizeof(v), &v); return v; };
  auto GetUint32Array  = [&](uint32_t size)->std::vector<uint32_t> { std::vector<uint32_t> data(size); GetData(size * sizeof(data[0]), data.data()); return data; };
  auto GetFloat32Array = [&](uint32_t size)->std::vector<float>    { std::vector<float>    data(size); GetData(size * sizeof(data[0]), data.data()); return data; };
  auto GetString       = [&]()->std::string                        { std::string s(GetUint32(), ' ');  GetData((uint32_t)s.size(), (void*)s.c_str()); return s; };

  // Read in the file.
  Assert(BMF_HAS_BMF == GetUint32(), "Not a BMF file");

  // Read global positions.
  AssertEq(GetUint32(), BMF_HAS_POSITIONS);
  uint32_t positionCount = GetUint32();
  std::vector<float> positions = GetFloat32Array(3 * positionCount);
  AssertEq(GetUint32(), BMF_GOT_POSITIONS);

  // Read global colors.
  std::vector<float> colors;
  uint32_t d = GetUint32();
  if (d == BMF_HAS_COLORS)
  {
    uint32_t colorCount = GetUint32();
    AssertEq(colorCount, positionCount);
    colors = GetFloat32Array(3 * colorCount),
    AssertEq(GetUint32(), BMF_GOT_COLORS),
    d = GetUint32();
  }

  while (d == BMF_HAS_MESH)
  {
    BMF_Geometry geometry;

    // Read name.
    d = GetUint32();
    if (d == BMF_HAS_NAME)
    {
      geometry.name = GetString();
      d = GetUint32();
      AssertEq(d, BMF_GOT_NAME);

      for (const BMF_Geometry& g : object.geometries)
        if (g.name == geometry.name)
          geometry.name += "." + std::to_string(object.geometries.size());
    }
    else
      geometry.name = "Object." + std::to_string(object.geometries.size());

    // Read Indices..
    uint32_t triangleCount = GetUint32();
    geometry.indices = GetUint32Array(3 * triangleCount);
    AssertEq(GetUint32(), BMF_GOT_INDICES);

    // Read UVs.
    d = GetUint32();
    if (d == BMF_HAS_UVS)
    {
      uint32_t uvCount = GetUint32();
      AssertEq(uvCount, 3 * triangleCount);
      geometry.uvs = GetFloat32Array(2 * uvCount);
      AssertEq(GetUint32(), BMF_GOT_UVS);
      d = GetUint32();
    }

    // Read normals.
    if (d == BMF_HAS_NORMALS)
    {
      uint32_t normalCount = GetUint32();
      AssertEq(normalCount, 3 * triangleCount);
      geometry.normals = GetFloat32Array(3 * normalCount);
      AssertEq(GetUint32(), BMF_GOT_NORMALS);
      d = GetUint32();
    }

    // End of mesh.
    AssertEq(d, BMF_GOT_MESH);
    d = GetUint32();

    // Process mesh.
    AssertEq((uint32_t)geometry.indices.size(), 3 * triangleCount);

    // Process positions.
    geometry.positions.resize(3 * geometry.indices.size());
    for (uint32_t k = 0; k < geometry.indices.size(); k += 1)
    {
      // NOTE: We flip the y and the z components because y should always be up.
      Assert(geometry.indices[k] < positions.size(), "Vertex index out of range");
      geometry.positions[3 * k + 0] = positions[3 * geometry.indices[k] + 0];
      geometry.positions[3 * k + 1] = positions[3 * geometry.indices[k] + 2];
      geometry.positions[3 * k + 2] = positions[3 * geometry.indices[k] + 1];
    }

    // Process colors.
    if (colors.size())
    {
      colors.resize(3 * geometry.indices.size());
      for (uint32_t k = 0; k < geometry.indices.size(); ++k)
      {
        Assert(geometry.indices[k] < colors.size(), "Color index out of range"),
        geometry.colors[3 * k + 0] = colors[3 * geometry.indices[k] + 0],
        geometry.colors[3 * k + 1] = colors[3 * geometry.indices[k] + 1],
        geometry.colors[3 * k + 2] = colors[3 * geometry.indices[k] + 2];
      }
    }

    object.geometries.push_back(geometry);
  }

  AssertEq(d, BMF_GOT_BMF);

  fclose(pFile);
}

int main(int argc, char** ppArgv)
{
  if (argc < 3)
  {
    printf("USAGE: [INPUT_FILE(S)] [OUTPUT_FILE]\n");
    return -1;
  }
  else
  {
    printf("Reading in:\n");
    for (int i = 1; i < argc - 1; ++i)
      printf("%s\n", ppArgv[i]);
    printf("\nOutputting to:\n");
    printf("%s\n", ppArgv[argc - 1]);

  }

  BMF_Object object;
  for (int i = 1; i < argc - 1; ++i)
    Read(object, ppArgv[i]);
  ExportToOBJ(object, ppArgv[argc - 1]);

  return 0;
}
