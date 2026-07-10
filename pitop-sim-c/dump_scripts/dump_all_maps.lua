-- dump_all_maps.lua — Dump automatique de toutes les cartes de Pokémon Saphir
--
-- PRINCIPE :
--   Le jeu stocke dans la ROM une table `gMapLayouts` : un tableau de
--   pointeurs vers des structures MapLayout, une par carte.
--   Chaque MapLayout contient les dimensions de la carte, ses tilesets,
--   et des pointeurs vers les données (grille de métatiles, warps, etc.)
--
-- UTILISATION :
--   1. Lancer BizHawk + Pokémon Saphir (USA/Europe v1.1 / Rev 1)
--   2. Tools → Lua Console → ouvrir ce script
--   3. Les fichiers sont générés dans data/maps/
--   4. Générer l'index : lua gen_index.lua (ou fait automatiquement)
--
-- STRUCTURES ROM (d'après le decompilateur pokeemerald) :
--
--   struct MapLayout {
--       u16 width;              // +0 : largeur en tiles
--       u16 height;             // +2 : hauteur en tiles
--       u16 primary_tileset;    // +4 : ID tileset primaire
--       u16 secondary_tileset;  // +6 : ID tileset secondaire
--       u8  border_width;       // +8
--       u8  border_height;      // +9
--       u16 *border;            // +10 : pointeur tiles bordure
--       u16 *metatiles;         // +14 : pointeur grille métatiles
--       struct WarpData *warps; // +18 : pointeur warps
--       struct CoordEvent *coord_events; // +22
--       struct BgEvent *bg_events;       // +26
--       struct MapConnection *connections; // +30
--       u16 music;              // +34
--       u8  weather;            // +36
--       u8  map_type;           // +37
--       u8  padding[2];         // +38
--   };  // total = 40 octets
--
--   struct WarpData {
--       s16 x, y;       // +0,+2
--       u8 elevation;   // +4
--       u8 warp_id;     // +5
--       u8 map_group;   // +6
--       u8 map_num;     // +7
--   };  // total = 8 octets
--
--   struct CoordEvent {
--       u16 x, y;       // +0,+2
--       u8 elevation;   // +4
--       u8 trigger_type;// +5
--       u16 script_index;// +6
--   };  // total = 8 octets
--
--   struct BgEvent {
--       u16 x, y;       // +0,+2
--       u8 elevation;   // +4
--       u8 kind;        // +5
--       u16 script_id;  // +6
--   };  // total = 8 octets
--
--   struct MapConnection {
--       u8 direction;   // +0 : 1=SOUTH 2=NORTH 3=WEST 4=EAST
--       u8 offset;      // +1
--       u8 map_group;   // +2
--       u8 map_num;     // +3
--   };  // total = 4 octets
--
-- Adresses ROM connues pour gMapLayouts :
--   Pokémon Saphir (U) v1.0 : 0x082D3A64
--   Pokémon Saphir (U) v1.1 : 0x082D3A64 (inchangé)
--   Pokémon Rubis  (U) v1.0 : 0x082D3914
--   Pokémon Rubis  (U) v1.1 : 0x082D3914
--   Pokémon Émeraude (U)    : 0x082E058C
--   Pokémon Saphir (Eur) v1.1 : 0x082D3A64 (même rom US/Eur)
--
-- Si l'adresse est fausse, le script tente une détection automatique
-- en cherchant dans la ROM une table de pointeurs valides.

local OUTPUT_DIR = "data/maps/"
local INDEX_FILE = "data/index.csv"

-- ======================================================================
-- 1. CONFIGURATION
-- ======================================================================

-- Adresses possibles de gMapLayouts à essayer (par ordre de priorité)
local CANDIDATE_ADDRESSES = {
    0x082D3A64,  -- Saphir (U/Eur) v1.0, v1.1
    0x082D3914,  -- Rubis (U) v1.0, v1.1
    0x082E058C,  -- Émeraude (U)
    0x08347520,  -- Alternative Saphir
    0x083D3A64,  -- Alternative hautes adresses
}

-- Adresse fixe (si connue) ; laisser nil pour auto-détection
local FIXED_ADDRESS = 0x082D3A64

-- ======================================================================
-- 2. FONCTIONS UTILITAIRES DE LECTURE MÉMOIRE
-- ======================================================================

local function read_byte(addr, region)
    return memory.readbyte(addr, region)
end

local function read_word(addr, region)
    local lo = read_byte(addr, region)
    local hi = read_byte(addr + 1, region)
    if not lo or not hi then return nil end
    return lo + hi * 256
end

local function read_long(addr, region)
    local b0 = read_byte(addr, region)
    local b1 = read_byte(addr + 1, region)
    local b2 = read_byte(addr + 2, region)
    local b3 = read_byte(addr + 3, region)
    if not b0 or not b1 or not b2 or not b3 then return nil end
    return b0 + b1 * 256 + b2 * 65536 + b3 * 16777216
end

-- ======================================================================
-- 3. DÉTECTION AUTOMATIQUE DE LA TABLE gMapLayouts
-- ======================================================================

-- Vérifie si un pointeur pointe vers un MapLayout valide.
-- Un MapLayout valide a :
--   - width, height entre 1 et 64
--   - tilesets entre 0 et 20
--   - les pointeurs (metatiles, warps) pointent dans la ROM
local function is_valid_layout_ptr(ptr)
    if not ptr or ptr < 0x08000000 or ptr > 0x0A000000 then
        return false
    end

    local w  = read_word(ptr, "ROM")
    local h  = read_word(ptr + 2, "ROM")
    local pt = read_word(ptr + 4, "ROM")
    local st = read_word(ptr + 6, "ROM")

    if not w or not h or not pt or not st then return false end
    if w < 1 or w > 128 or h < 1 or h > 128 then return false end
    if pt > 30 or st > 30 then return false end

    -- Vérifier que le pointeur metatiles pointe dans la ROM
    local meta_ptr = read_long(ptr + 14, "ROM")
    if not meta_ptr then return false end
    if meta_ptr < 0x08000000 or meta_ptr > 0x0A000000 then return false end

    return true
end

-- Scanne la ROM pour trouver la table gMapLayouts.
-- gMapLayouts est un tableau de pointeurs vers des MapLayout.
-- On cherche une zone de la ROM où N pointeurs consécutifs pointent
-- vers des MapLayout valides.
local function find_layout_table(search_start, search_end)
    search_start = search_start or 0x08000000
    search_end   = search_end   or 0x09000000

    print(string.format("Recherche de gMapLayouts entre 0x%08X et 0x%08X...",
          search_start, search_end))

    local step = 4  -- Les pointeurs sont alignés sur 4 octets
    local addr = search_start

    while addr < search_end do
        local ptr = read_long(addr, "ROM")
        if ptr and ptr >= 0x08000000 and ptr <= 0x0A000000 then
            if is_valid_layout_ptr(ptr) then
                -- Vérifier les entrées suivantes
                local valid_count = 1
                for j = 1, 20 do
                    local next_ptr = read_long(addr + j * 4, "ROM")
                    if next_ptr and is_valid_layout_ptr(next_ptr) then
                        valid_count = valid_count + 1
                    else
                        break
                    end
                end
                if valid_count >= 10 then
                    print(string.format("  → gMapLayouts trouvée à 0x%08X (%d layouts valides consécutifs)",
                          addr, valid_count))
                    return addr
                end
            end
        end
        addr = addr + step
    end

    return nil
end

-- ======================================================================
-- 4. LECTURE D'UNE CARTE
-- ======================================================================

-- Lit et retourne les données d'une carte.
-- map_group, map_num : identifiants de la carte
-- layout_table_addr  : adresse ROM de gMapLayouts
local function read_map(map_group, map_num, layout_table_addr)
    local entry_offset = layout_table_addr + (map_group * 256 + map_num) * 4
    local layout_ptr = read_long(entry_offset, "ROM")

    if not layout_ptr or layout_ptr < 0x08000000 or layout_ptr > 0x0A000000 then
        return nil
    end

    -- MapLayout header (40 octets)
    local w  = read_word(layout_ptr, "ROM")
    local h  = read_word(layout_ptr + 2, "ROM")
    if not w or not h or w == 0 or h == 0 then return nil end

    local primary_ts   = read_word(layout_ptr + 4, "ROM")
    local secondary_ts = read_word(layout_ptr + 6, "ROM")
    local meta_ptr     = read_long(layout_ptr + 14, "ROM")
    local warp_ptr     = read_long(layout_ptr + 18, "ROM")
    local coord_ptr    = read_long(layout_ptr + 22, "ROM")
    local bg_ptr       = read_long(layout_ptr + 26, "ROM")
    local conn_ptr     = read_long(layout_ptr + 30, "ROM")

    if not meta_ptr or meta_ptr < 0x08000000 then return nil end

    -- Lire la grille de métatiles
    local metatiles = {}
    for y = 0, h - 1 do
        metatiles[y] = {}
        for x = 0, w - 1 do
            local addr = meta_ptr + (y * w + x) * 2
            metatiles[y][x] = read_word(addr, "ROM")
        end
    end

    -- Lire les warps
    local warps = {}
    if warp_ptr and warp_ptr >= 0x08000000 and warp_ptr < 0x0A000000 then
        for i = 0, 31 do
            local addr = warp_ptr + i * 8
            local wx = read_word(addr, "ROM")
            if not wx then break end
            -- Un warp valide a x,y != -1 et map_group dans [0,30]
            local wy     = read_word(addr + 2, "ROM")
            local elev   = read_byte(addr + 4, "ROM")
            local wid    = read_byte(addr + 5, "ROM")
            local wmg    = read_byte(addr + 6, "ROM")
            local wmn    = read_byte(addr + 7, "ROM")
            if not wmg then break end
            if wx >= 0 and wy >= 0 and wmg <= 30 then
                table.insert(warps, {
                    x = wx, y = wy,
                    dest_group = wmg,
                    dest_num   = wmn,
                    dest_x     = elev,  -- Note: elevation sert de dest_x
                    dest_y     = wid    -- warp_id sert de dest_y
                })
            else
                break
            end
        end
    end

    -- Lire les triggers (CoordEvent)
    local triggers = {}
    if coord_ptr and coord_ptr >= 0x08000000 and coord_ptr < 0x0A000000 then
        for i = 0, 31 do
            local addr = coord_ptr + i * 8
            local tx = read_word(addr, "ROM")
            if not tx then break end
            if tx >= 0x08000000 then break end  -- pointeur ou données invalides
            local ty     = read_word(addr + 2, "ROM")
            local elev   = read_byte(addr + 4, "ROM")
            local ttype  = read_byte(addr + 5, "ROM")
            local script = read_word(addr + 6, "ROM")
            if not ttype then break end
            if tx >= 0 and ty >= 0 and ttype <= 10 then
                table.insert(triggers, {
                    x = tx, y = ty,
                    type = ttype,
                    script_id = script
                })
            else
                break
            end
        end
    end

    -- Lire les connexions
    local connections = {}
    if conn_ptr and conn_ptr >= 0x08000000 and conn_ptr < 0x0A000000 then
        for i = 0, 3 do
            local addr = conn_ptr + i * 4
            local dir = read_byte(addr, "ROM")
            if not dir then break end
            if dir < 1 or dir > 4 then break end
            local off = read_byte(addr + 1, "ROM")
            local cmg = read_byte(addr + 2, "ROM")
            local cmn = read_byte(addr + 3, "ROM")
            if off == 0xFF or cmg == nil then break end
            table.insert(connections, {
                dir = dir,
                offset = off,
                dest_group = cmg,
                dest_num   = cmn
            })
        end
    end

    return {
        map_group = map_group,
        map_num = map_num,
        width = w,
        height = h,
        primary_tileset = primary_ts or 0,
        secondary_tileset = secondary_ts or 0,
        metatiles = metatiles,
        warps = warps,
        triggers = triggers,
        connections = connections
    }
end

-- ======================================================================
-- 5. EXPORT
-- ======================================================================

local function export_map(data, path)
    local file = io.open(path, "w")
    if not file then
        print("  ERREUR: impossible d'écrire " .. path)
        return false
    end

    file:write(string.format("%d %d %d %d\n",
        data.width, data.height,
        data.primary_tileset, data.secondary_tileset))

    file:write(string.format("%d\n", #data.connections))
    local dir_names = { "", "SOUTH", "NORTH", "WEST", "EAST" }
    for _, conn in ipairs(data.connections) do
        local dname = dir_names[conn.dir] or tostring(conn.dir)
        file:write(string.format("%s %d %d %d\n",
            dname, conn.dest_group, conn.dest_num, conn.offset or 0))
    end

    file:write(string.format("%d\n", #data.warps))
    for _, warp in ipairs(data.warps) do
        local hole = warp.hole and " hole" or ""
        file:write(string.format("%d %d %d %d %d %d%s\n",
            warp.x, warp.y,
            warp.dest_group, warp.dest_num,
            warp.dest_x or 0, warp.dest_y or 0, hole))
    end

    file:write(string.format("%d\n", #data.triggers))
    for _, trig in ipairs(data.triggers) do
        file:write(string.format("%d %d %d %d\n",
            trig.x, trig.y, trig.type, trig.script_id or 0))
    end

    for y = 0, data.height - 1 do
        for x = 0, data.width - 1 do
            local v = data.metatiles[y] and data.metatiles[y][x]
            file:write(tostring(v or 0))
            if x < data.width - 1 then file:write(" ") end
        end
        file:write("\n")
    end

    file:close()
    return true
end

-- ======================================================================
-- 6. BOUCLE PRINCIPALE
-- ======================================================================

print("=== PiTOP — Dump automatique des cartes ===")
print("")

-- Créer le répertoire de sortie
os.execute("mkdir -p " .. OUTPUT_DIR)

-- Trouver l'adresse de gMapLayouts
local layout_table_addr = FIXED_ADDRESS

if not layout_table_addr then
    -- Essayer les adresses candidates connues
    for _, addr in ipairs(CANDIDATE_ADDRESSES) do
        local test_ptr = read_long(addr, "ROM")
        if test_ptr and is_valid_layout_ptr(test_ptr) then
            -- Vérifier que l'entrée suivante est aussi valide
            local next_ptr = read_long(addr + 4, "ROM")
            if next_ptr and is_valid_layout_ptr(next_ptr) then
                layout_table_addr = addr
                print(string.format("Adresse valide trouvée : 0x%08X", addr))
                break
            end
        end
    end
end

if not layout_table_addr then
    print("Aucune adresse connue valide. Lancement de la détection automatique...")
    layout_table_addr = find_layout_table(0x08010000, 0x08500000)
end

if not layout_table_addr then
    print("")
    print("============================================================")
    print("ERREUR : impossible de trouver gMapLayouts dans la ROM.")
    print("")
    print("Causes possibles :")
    print("  1. ROM non chargée ou incompatible (pas Saphir/Rubis US/Eur)")
    print("  2. Adresse ROM différente de celle attendue")
    print("")
    print("Solution :")
    print("  - Ouvrir le Debug → Memory Viewer dans BizHawk")
    print("  - Chercher 'gMapLayouts' dans la ROM (offset du fichier .gba)")
    print("  - Modifier FIXED_ADDRESS en haut du script")
    print("  - Ou utiliser pokeruby/pokeemerald pour trouver l'adresse")
    print("============================================================")
    return
end

print(string.format("Table gMapLayouts à 0x%08X", layout_table_addr))
print("")

-- Itérer sur les groupes de cartes
-- Saphir a des map_group de 0 à environ 15, chaque groupe a ~200 cartes max
local total_exported = 0
local index = {}

for mg = 0, 20 do
    for mn = 0, 255 do
        local ok, data = pcall(read_map, mg, mn, layout_table_addr)
        if ok and data then
            local filename = string.format("%d_%d.txt", mg, mn)
            local path = OUTPUT_DIR .. filename
            if export_map(data, path) then
                print(string.format("  [%3d-%-3d] %dx%d ts=%d/%d warps=%d trigs=%d conns=%d → %s",
                    mg, mn,
                    data.width, data.height,
                    data.primary_tileset, data.secondary_tileset,
                    #data.warps, #data.triggers, #data.connections,
                    filename))
                table.insert(index, string.format("%d,%d,%s", mg, mn, filename))
                total_exported = total_exported + 1
            end
            emu.frameadvance()  -- Laisser l'émulateur respirer
        end
    end
end

-- Écrire l'index CSV
local idxfile = io.open(INDEX_FILE, "w")
if idxfile then
    idxfile:write("group,num,file\n")
    for _, line in ipairs(index) do
        idxfile:write(line .. "\n")
    end
    idxfile:close()
    print("")
    print(string.format("Index écrit : %s (%d entrées)", INDEX_FILE, #index))
end

print("")
print(string.format("=== Dump terminé : %d cartes exportées ===", total_exported))
print("")
print("Pour utiliser ces données avec le simulateur C :")
print(string.format("  cd pitop-sim-c && cp -r %s* data/maps/ && cp %s data/", OUTPUT_DIR, INDEX_FILE))
