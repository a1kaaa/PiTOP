-- Script ultime et universel pour BizHawk (.NET Sockets)
luanet.load_assembly("System")
local TcpClient = luanet.import_type("System.Net.Sockets.TcpClient")

local client = nil
local stream = nil

local success, err = pcall(function()
    client = TcpClient("127.0.0.1", 4444)
    client.NoDelay = true
    stream = client:GetStream()
end)

if not success or not client then
    print("❌ Impossible de se connecter au serveur Java.")
    if err then print("Détails : " .. tostring(err)) end
    return
else
    print("✅ Connecté au serveur Java ! Début du streaming...")
end

function get_current_action()
    local keys = joypad.get(1)
    if keys["Up"]    then return 0 end
    if keys["Down"]  then return 1 end
    if keys["Left"]  then return 2 end
    if keys["Right"] then return 3 end
    if keys["A"]     then return 4 end
    if keys["B"]     then return 5 end
    if keys["Start"] then return 6 end
    return 5 -- 'B' par défaut
end

-- Détection de la fonction de lecture de base de BizHawk
local mem = mainmemory or memory
local read_b = mem.readbyte or mem.read_u8 or mem.readbyteunsigned

if not read_b then
    print("❌ Erreur : Impossible de trouver la fonction de lecture RAM de BizHawk.")
    return
end

-- Boucle principale
while true do
    local status, loop_err = pcall(function()
        local action = get_current_action()

        -- 1. Lecture des octets simples (X, Y, Direction)
        local x = read_b(0x02024E4A)
        local y = read_b(0x02024E4C)
        local direction = read_b(0x02024E4E)

        -- 2. Reconstruction manuelle du Map ID (16-bits Little Endian)
        local map_id_low  = read_b(0x02024E6A)
        local map_id_high = read_b(0x02024E6B)
        local map_id      = map_id_low + (map_id_high * 256)

        -- 3. Reconstruction manuelle du RNG (32-bits Little Endian)
        local rng_1 = read_b(0x03005D80)
        local rng_2 = read_b(0x03005D81)
        local rng_3 = read_b(0x03005D82)
        local rng_4 = read_b(0x03005D83)
        local rng   = rng_1 + (rng_2 * 256) + (rng_3 * 65536) + (rng_4 * 16777216)

        -- 4. Envoi des 10 octets au serveur Java (Big Endian)
        stream:WriteByte(action % 256)
        stream:WriteByte(math.floor(map_id / 256) % 256)
        stream:WriteByte(map_id % 256)
        stream:WriteByte(x % 256)
        stream:WriteByte(y % 256)
        stream:WriteByte(direction % 256)
        stream:WriteByte(math.floor(rng / 16777216) % 256)
        stream:WriteByte(math.floor(rng / 65536) % 256)
        stream:WriteByte(math.floor(rng / 256) % 256)
        stream:WriteByte(rng % 256)
    end)

    if not status then
        print("❌ Erreur dans la boucle de capture : " .. tostring(loop_err))
        break
    end

    emu.frameadvance()
end