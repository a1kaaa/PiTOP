-- Script Pokémon Saphir - PI CONTROLLER (REVECEIVER MODE)
luanet.load_assembly("System")
local TcpClient = luanet.import_type("System.Net.Sockets.TcpClient")

local client, stream = nil, nil
local success = pcall(function()
    client = TcpClient("127.0.0.1", 4444)
    client.NoDelay = true
    stream = client:GetStream()
end)

if not success or not client then
    print("Serveur Java non détecté.")
    return
else
    print("Pi est aux commandes.")
end

local HOLD_FRAMES = 12 -- Temps d'appui sur la touche

while true do
    -- 1. ATTENDRE L'ORDRE DE JAVA (Bloquant)
    local action_id = stream:ReadByte()

    -- Si la connexion coupe (-1), on s'arrête
    if action_id == -1 or action_id == 255 then
        print("Connexion perdue avec le serveur Java.")
        break
    end

-- 2. TRADUIRE L'ACTION ID EN TOUCHES VIRTUELLES GBA
    local current_joypad = {}
    if action_id == 0 then current_joypad["Up"] = true
    elseif action_id == 1 then current_joypad["Down"] = true
    elseif action_id == 2 then current_joypad["Left"] = true
    elseif action_id == 3 then current_joypad["Right"] = true
    elseif action_id == 4 then current_joypad["A"] = true      -- Vrai bouton A de la console
    elseif action_id == 5 then current_joypad["B"] = true      -- Vrai bouton B de la console
    elseif action_id == 6 then current_joypad["Start"] = true  -- Vrai bouton Start (Menu)
    -- Si action_id >= 7 : Pas d'input (Pause)
    end

    -- 3. MAINTENIR L'INPUT PENDANT N FRAMES
    for i = 1, HOLD_FRAMES do
        joypad.set(current_joypad)
        emu.frameadvance()
    end

    -- 4. LIRE LA RAM ET RENVOYER LA TÉLÉMÉTRIE À JAVA
    local x         = memory.readbyte(0x25734, "EWRAM")
    local y         = memory.readbyte(0x25736, "EWRAM")
    local map_group = memory.readbyte(0x2572F, "EWRAM")
    local map_num   = memory.readbyte(0x25730, "EWRAM")

    local rng_1 = memory.readbyte(0x48C4, "IWRAM")
    local rng_2 = memory.readbyte(0x48C5, "IWRAM")
    local rng_3 = memory.readbyte(0x48C6, "IWRAM")
    local rng_4 = memory.readbyte(0x48C7, "IWRAM")

    local safe_direction = action_id <= 3 and action_id or 1

    -- Envoi du paquet de 10 octets de réponse
    pcall(function()
        stream:WriteByte(action_id % 256)
        stream:WriteByte(map_group % 256)
        stream:WriteByte(map_num % 256)
        stream:WriteByte(x % 256)
        stream:WriteByte(y % 256)
        stream:WriteByte(safe_direction)
        stream:WriteByte(rng_4 % 256)
        stream:WriteByte(rng_3 % 256)
        stream:WriteByte(rng_2 % 256)
        stream:WriteByte(rng_1 % 256)
    end)
end