//-----------------------------------------------------------------------------
// Copyright (C) Doppelgamer LLC, 2020
// All rights reserved.
//-----------------------------------------------------------------------------
#include "pch.h"

using namespace std::filesystem;

enum
{
   black,
   navy,
   forest,
   teal,
   maroon,
   purple,
   ochre,
   silver,

   gray,
   blue,
   green,
   cyan,
   red,
   magenta,
   yellow,
   white,
};

constexpr int dark = -8;
constexpr int bright = 8;

static_assert(dark+gray == black);
static_assert(dark+blue == navy);
static_assert(dark+green == forest);
static_assert(dark+cyan == teal);
static_assert(dark+red == maroon);
static_assert(dark+magenta == purple);
static_assert(dark+yellow == ochre);
static_assert(dark+white == silver);

using SizePalette = vector<umax>;

const SizePalette size_palette
{
   teal,       1_KB,
   forest,     1_MB,
   ochre,      10_MB,
   yellow,     50_MB,
   maroon,     100_MB,
   red,        1_GB,  
   magenta,
};

int GetSizeColor(umax sz, const SizePalette& palette=size_palette)
{
   umax color = palette.back();
   for (size_t i=1; i<palette.size(); i+=2)
   {
      if (sz < palette[i])
      {
         color = palette[i-1];
         break;
      }
   }
   
   assert(color >= 0 && color < 16);
   return (int)color;
}

struct FileTypeInfo
{
   cstr pre;
   cstr post;
   int color;
};

static const unordered_map<file_type, FileTypeInfo> infos
{
   {file_type::none,       {"?", "?", silver}},
   {file_type::not_found,  {"?", "?", red}},
   {file_type::regular,    {"" , "" , white}},
   {file_type::directory,  {"<", ">", cyan}},
   {file_type::symlink,    {"?", "?", yellow}},
   {file_type::block,      {"?", "?", magenta}},
   {file_type::character,  {"?", "?", gray}},
   {file_type::fifo,       {"?", "?", forest}},
   {file_type::socket,     {"?", "?", 3}},
   {file_type::unknown,    {"?", "?", 2}},
   {file_type::junction,   {"?", "?", silver}},
};

constexpr size_t tab_size = 3;
const string tab(tab_size, ' ');
constexpr bool use_delims = false;

struct Stats
{
   umax count = 0;
   umax size = 0;
   umax ondisk = 0;

   void Add(umax bytes, umax disk) { count++; size+=bytes; ondisk+=disk; }

   umax Avg() const { return (umax)round(size / (double)count); }
};

struct FileStats
{
   unordered_map<file_type, Stats> bytype;
   unordered_map<wstring, Stats> byext;
   Stats total;

   void Add(file_type type, const path& path, const wstring& ext, umax size, umax ondisk)
   {
      total.Add(size, ondisk);
      byext[ext].Add(size, ondisk);
      bytype[type].Add(size, ondisk);
   }
};

struct FileInfo
{
   file_type type = file_type::none;
   path path;
   umax size = 0;
   umax ondisk = 0;
};

HANDLE console = nullptr;

void SetColor(int color) { SetConsoleTextAttribute(console, color); }
void SetPos(COORD pos) { SetConsoleCursorPosition(console, pos); }
void SetPos(short x, short y) { SetPos({x, y}); }

COORD GetPos()
{   
   CONSOLE_SCREEN_BUFFER_INFO screen;
   GetConsoleScreenBufferInfo(console, &screen);
   return screen.dwCursorPosition;
}

COORD GetSize()
{
   CONSOLE_SCREEN_BUFFER_INFO screen;
   GetConsoleScreenBufferInfo(console, &screen);
   return screen.dwSize;
}

void Clear()
{
   COORD topLeft  = {0, 0};
   CONSOLE_SCREEN_BUFFER_INFO screen;
   DWORD written;

   GetConsoleScreenBufferInfo(console, &screen);
   FillConsoleOutputCharacterA(console, ' ', screen.dwSize.X * screen.dwSize.Y, topLeft, &written);
   FillConsoleOutputAttribute(console, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE, screen.dwSize.X * screen.dwSize.Y, topLeft, &written);
   SetConsoleCursorPosition(console, screen.dwCursorPosition);
}

void Write(COORD pos, cstr s)
{
   SetPos(pos);
   DWORD written;
   auto width = GetSize().X;
   auto ss = sformat("%*s\n", width, s);
   WriteConsoleA(console, s, width, &written, nullptr);
}

cstr BytesStr(umax bytes)
{
   char tbuf[32];
   int slen = sprintf_s(tbuf, "%ju", bytes);
   char* o = tbuf;
   assert(slen <= 25);
   cstr bytestr = sformat_ptr();
   for (int i=0; i<slen; i++)
   {
      int dist = slen - i - 1;
      bool comma = dist % 3 == 0 && isdigit(*o) && isdigit(*(o+1));

      sformat_put(*o++);
      if (comma) sformat_put(',');
   }
   sformat_put(' ');
   sformat_put('B');
   sformat_put(0);
   return bytestr;
}

cstr SizeStr(umax bytes)
{
   return bytes < MB ? sformat("%.2f KB", bytes / (double)KB) :
          bytes < GB ? sformat("%.2f MB", bytes / (double)MB) :
                       sformat("%.2f GB", bytes / (double)GB);
}

pair<cstr, cstr> GetSizeStr(umax bytes)
{
   return {BytesStr(bytes), SizeStr(bytes)};
}

size_t size_on_disk(cstr filename)
{
   DWORD high;
   DWORD low = GetCompressedFileSizeA(filename, &high);
   
   if constexpr (cpu_bits == 64) 
      return ((size_t)high << 32) | (size_t)low;
   else
      return low;
}

int main(int argc, char *argv[])
{
   console = GetStdHandle(STD_OUTPUT_HANDLE);
   bool walk = false;
   size_t topcount = 500;
   vector<string> targets;

   for (int i=1; i<argc; i++)
   {
      printf("  [%d]: %s\n", i, argv[i]);
      auto len = strlen(argv[i]);
      if (argv[i][0]     == '\"')
         argv[i]++;
      if (argv[i][len-1] == '\"')
         argv[i][len-1] = 0;

      string arg = ToLower(argv[i]);

      if (arg == "-walk")
         walk = true;
      else
         targets.push_back(arg);
   }

   if (targets.empty())
      targets.emplace_back(current_path().string());

   FileStats stats;
   vector<FileInfo> files;
   unordered_map<file_type, vector<FileInfo>> filesByType;
   unordered_map<wstring, vector<FileInfo>> filesByExt;

   auto basey = GetPos().Y;

   for (const path& root: targets)
   {
      for (recursive_directory_iterator dir(root), end; dir!=end; ++dir)
      {
         try
         {
            const auto& entry = *dir;
            const auto depth = dir.depth();
            const auto path = entry.path();
            const auto name = path.filename().wstring();
            const auto ext = path.extension().wstring();
            const file_type type = entry.status().type();
            const auto bytes = entry.file_size();
            const auto ondisk = size_on_disk(path.string().c_str());

            if (!entry.is_directory())
            {
               FileInfo info {type, path, bytes};

               stats.Add(type, path, ext, bytes, ondisk);
               files.push_back(info);
               filesByType[type].push_back(info);
               filesByExt[ext].push_back(info);
            }

            if (walk)
            {
               static string indent(512, ' ');

               indent.resize(tab_size * depth, ' ');
               if (indent.size() >= tab_size)
                  indent[indent.size()-tab_size] = '|';

               auto [bytesbuf, sizebuf] = GetSizeStr(bytes);
               auto [pre, post, color] = infos.at(type);
               SetColor(color);

               int sizecolor;

               if (entry.is_directory())
               {
                  sizebuf = "";
                  bytesbuf = "<DIR>";
                  sizecolor = gray;
               }
               else
               {
                  sizecolor = GetSizeColor(bytes);
               }

               SetColor(sizecolor);
               printf("%12s %25s", sizebuf, bytesbuf);
               printf("%s", tab.c_str());
               SetColor(gray);
               printf("%s", indent.c_str());
               SetColor(color);
               if constexpr (use_delims)
                  printf("%s%ls%s", pre, name.c_str(), post);
               else
                  printf("%ls", name.c_str());
               printf("\n");
            }
            else
            {
               auto y = basey;
               SetColor(white);
               Write({0, y++}, sformat("file: %s", path.string().c_str()));
               Write({0, y++}, sformat("count: %s", str(stats.total.count)));
               Write({0, y++}, sformat("logical size: %s (%s)", SizeStr(stats.total.size), BytesStr(stats.total.size)));
               SetColor(cyan);
               Write({0, y++}, sformat("size on disk: %s (%s)", SizeStr(stats.total.ondisk), BytesStr(stats.total.ondisk)));
            }
         }
         catch (const exception& e)
         {
            SetColor(red);
            printf("ERROR: %s\n", e.what());
         }
      }
   }

   Clear();
   SetColor(white);

   struct SizePair
   {
      cstr header[2];
      umax(*func)(const Stats&);
   };

   vector<pair<wstring, Stats>> exts(stats.byext.begin(), stats.byext.end());
   sort(exts.begin(), exts.end(), [](const auto& a, const auto& b){ return a.second.size > b.second.size; });

   static constexpr cstr prefmt = "  %-26ls %8s";
   static constexpr cstr numfmt[] {" %18s", " %16s"};

   const SizePair pairs[]
   {
      {{"bytes",         "size"},         [](const Stats& s){ return s.size; }},
      {{"avg bytes",     "avg size"},     [](const Stats& s){ return s.Avg(); }},
      {{"bytes on disk", "size on disk"}, [](const Stats& s){ return s.ondisk; }},
   };

   string header;
   header += sformat(prefmt, L"ext", "count");
   for (const auto& s: pairs)
      loopi(2)
         header += sformat(numfmt[i], s.header[i]);

   const size_t linewidth = header.size();
   static const string line(linewidth, '-');

   printf("\n");
   SetColor(white);
   printf("%s\n", header.c_str());
   SetColor(gray);
   printf("%s\n", line.c_str());
   
   for (const auto& e: exts)
   {
      SetColor(white);
      printf(prefmt, e.first.c_str(), str(e.second.count));
      for (const auto& p: pairs)
      {
         auto sz = p.func(e.second);
         auto [bytes, size] = GetSizeStr(sz);
         SetColor(gray);
         printf(numfmt[0], bytes);
         SetColor(GetSizeColor(sz));
         printf(numfmt[1], size);
      }
      printf("\n");
   }

   sort(files.begin(), files.end(), [](const auto& a, const auto& b){ return a.size > b.size; });
   if (files.size() > topcount)
      files.resize(topcount);
   printf("\n\n");
   printf("Top %s files:\n", str(topcount));
   printf("%s\n", line.c_str());
   for (const auto& f: files)
   {
      SetColor(gray);
      printf("  %16s", BytesStr(f.size));
      SetColor(GetSizeColor(f.size));
      printf(" %16s     ", SizeStr(f.size));
      SetColor(white);
      printf("%ls\n", f.path.wstring().c_str());
   }

   system("pause");
   return 0;
}

