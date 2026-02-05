#include "pch.h"
#include "NetworkPackets.h"

//=============================================================================
// ClientInputPacket
//=============================================================================

ClientInputPacket::ClientInputPacket()
    : clientId(255)
    , teamId(255)
    , inputSequence(0)
    , lastServerTick(0)
    , aimPosition()
    , inputFlags(0)
{}

void ClientInputPacket::Write(DataWriter& writer) const
{
    writer.WriteByte(clientId);
    writer.WriteByte(teamId);
    writer.WriteUInt32(inputSequence);
    writer.WriteUInt32(lastServerTick);
    writer.WriteInt16(aimPosition.x);
    writer.WriteInt16(aimPosition.y);
    writer.WriteInt16(aimPosition.z);
    writer.WriteUInt16(inputFlags);
}

void ClientInputPacket::Read(DataReader& reader)
{
    clientId = reader.Read<uint8_t>();
    teamId = reader.Read<uint8_t>();
    inputSequence = reader.Read<uint32_t>();
    lastServerTick = reader.Read<uint32_t>();
    aimPosition.x = reader.Read<int16_t>();
    aimPosition.y = reader.Read<int16_t>();
    aimPosition.z = reader.Read<int16_t>();
    inputFlags = reader.Read<uint16_t>();
}

//=============================================================================
// EntityStateDelta
//=============================================================================

EntityStateDelta::EntityStateDelta()
    : entityId(INVALID_NET_ENTITY_ID)
    , entityType(0)
    , stateFlags(0)
    , position()
    , velocity()
    , health(0)
    , state(0)
    , targetId(INVALID_NET_ENTITY_ID)
    , orders(0)
{}

void EntityStateDelta::Write(DataWriter& writer) const
{
    writer.WriteUInt16(entityId);
    writer.WriteByte(entityType);
    writer.WriteByte(stateFlags);
    
    if (stateFlags & EntityStateFlags::Position)
    {
        writer.WriteInt16(position.x);
        writer.WriteInt16(position.y);
        writer.WriteInt16(position.z);
    }
    
    if (stateFlags & EntityStateFlags::Velocity)
    {
        writer.WriteByte(velocity.x);
        writer.WriteByte(velocity.y);
        writer.WriteByte(velocity.z);
    }
    
    if (stateFlags & EntityStateFlags::Health)
    {
        writer.WriteUInt16(health);
    }
    
    if (stateFlags & EntityStateFlags::State)
    {
        writer.WriteByte(state);
    }
    
    if (stateFlags & EntityStateFlags::Target)
    {
        writer.WriteUInt16(targetId);
    }
    
    if (stateFlags & EntityStateFlags::Orders)
    {
        writer.WriteByte(orders);
    }
}

void EntityStateDelta::Read(DataReader& reader)
{
    entityId = reader.Read<uint16_t>();
    entityType = reader.Read<uint8_t>();
    stateFlags = reader.Read<uint8_t>();
    
    if (stateFlags & EntityStateFlags::Position)
    {
        position.x = reader.Read<int16_t>();
        position.y = reader.Read<int16_t>();
        position.z = reader.Read<int16_t>();
    }
    
    if (stateFlags & EntityStateFlags::Velocity)
    {
        velocity.x = reader.Read<int8_t>();
        velocity.y = reader.Read<int8_t>();
        velocity.z = reader.Read<int8_t>();
    }
    
    if (stateFlags & EntityStateFlags::Health)
    {
        health = reader.Read<uint16_t>();
    }
    
    if (stateFlags & EntityStateFlags::State)
    {
        state = reader.Read<uint8_t>();
    }
    
    if (stateFlags & EntityStateFlags::Target)
    {
        targetId = reader.Read<uint16_t>();
    }
    
    if (stateFlags & EntityStateFlags::Orders)
    {
        orders = reader.Read<uint8_t>();
    }
}

size_t EntityStateDelta::EstimateSize() const
{
    size_t size = 4; // entityId + entityType + stateFlags
    
    if (stateFlags & EntityStateFlags::Position) size += 6;
    if (stateFlags & EntityStateFlags::Velocity) size += 3;
    if (stateFlags & EntityStateFlags::Health) size += 2;
    if (stateFlags & EntityStateFlags::State) size += 1;
    if (stateFlags & EntityStateFlags::Target) size += 2;
    if (stateFlags & EntityStateFlags::Orders) size += 1;
    
    return size;
}

//=============================================================================
// StateSnapshotPacket
//=============================================================================

StateSnapshotPacket::StateSnapshotPacket()
    : serverTick(0)
    , ackInputSequence(0)
    , entityCount(0)
{}

void StateSnapshotPacket::Write(DataWriter& writer) const
{
    writer.WriteUInt32(serverTick);
    writer.WriteUInt32(ackInputSequence);
    writer.WriteUInt16(static_cast<uint16_t>(entities.size()));
    
    for (const auto& entity : entities)
    {
        entity.Write(writer);
    }
}

void StateSnapshotPacket::Read(DataReader& reader)
{
    serverTick = reader.Read<uint32_t>();
    ackInputSequence = reader.Read<uint32_t>();
    entityCount = reader.Read<uint16_t>();
    
    entities.clear();
    entities.reserve(entityCount);
    
    for (uint16_t i = 0; i < entityCount; ++i)
    {
        EntityStateDelta delta;
        delta.Read(reader);
        entities.push_back(delta);
    }
}

bool StateSnapshotPacket::CanAddEntity(const EntityStateDelta& delta) const
{
    return EstimateSize() + delta.EstimateSize() <= NET_MAX_PAYLOAD_SIZE - sizeof(NetPacketHeader);
}

void StateSnapshotPacket::AddEntity(const EntityStateDelta& delta)
{
    entities.push_back(delta);
    entityCount = static_cast<uint16_t>(entities.size());
}

size_t StateSnapshotPacket::EstimateSize() const
{
    size_t size = HEADER_SIZE;
    for (const auto& entity : entities)
    {
        size += entity.EstimateSize();
    }
    return size;
}

//=============================================================================
// CommandPacket
//=============================================================================

CommandPacket::CommandPacket()
    : commandId(0)
    , teamId(255)
    , commandType(CommandType::None)
    , entityType(0)
    , count(0)
    , buildingId(-1)
    , targetEntityId(INVALID_NET_ENTITY_ID)
    , targetPosition()
    , programId(0)
{}

void CommandPacket::Write(DataWriter& writer) const
{
    writer.WriteUInt32(commandId);
    writer.WriteByte(teamId);
    writer.WriteByte(static_cast<uint8_t>(commandType));
    
    switch (commandType)
    {
    case CommandType::CreateUnit:
        writer.WriteByte(entityType);
        writer.WriteUInt16(count);
        writer.WriteInt32(buildingId);
        writer.WriteInt16(targetPosition.x);
        writer.WriteInt16(targetPosition.y);
        writer.WriteInt16(targetPosition.z);
        break;
        
    case CommandType::SelectUnit:
        writer.WriteUInt16(targetEntityId);
        break;
        
    case CommandType::MoveUnit:
    case CommandType::AttackTarget:
        writer.WriteInt16(targetPosition.x);
        writer.WriteInt16(targetPosition.y);
        writer.WriteInt16(targetPosition.z);
        writer.WriteUInt16(targetEntityId);
        break;
        
    case CommandType::AimBuilding:
        writer.WriteInt32(buildingId);
        writer.WriteInt16(targetPosition.x);
        writer.WriteInt16(targetPosition.y);
        writer.WriteInt16(targetPosition.z);
        break;
        
    case CommandType::ToggleFence:
        writer.WriteInt32(buildingId);
        break;
        
    case CommandType::RunProgram:
        writer.WriteByte(programId);
        break;
        
    case CommandType::TargetProgram:
        writer.WriteByte(programId);
        writer.WriteInt16(targetPosition.x);
        writer.WriteInt16(targetPosition.y);
        writer.WriteInt16(targetPosition.z);
        break;
        
    default:
        break;
    }
}

void CommandPacket::Read(DataReader& reader)
{
    commandId = reader.Read<uint32_t>();
    teamId = reader.Read<uint8_t>();
    commandType = static_cast<CommandType>(reader.Read<uint8_t>());
    
    switch (commandType)
    {
    case CommandType::CreateUnit:
        entityType = reader.Read<uint8_t>();
        count = reader.Read<uint16_t>();
        buildingId = reader.Read<int32_t>();
        targetPosition.x = reader.Read<int16_t>();
        targetPosition.y = reader.Read<int16_t>();
        targetPosition.z = reader.Read<int16_t>();
        break;
        
    case CommandType::SelectUnit:
        targetEntityId = reader.Read<uint16_t>();
        break;
        
    case CommandType::MoveUnit:
    case CommandType::AttackTarget:
        targetPosition.x = reader.Read<int16_t>();
        targetPosition.y = reader.Read<int16_t>();
        targetPosition.z = reader.Read<int16_t>();
        targetEntityId = reader.Read<uint16_t>();
        break;
        
    case CommandType::AimBuilding:
        buildingId = reader.Read<int32_t>();
        targetPosition.x = reader.Read<int16_t>();
        targetPosition.y = reader.Read<int16_t>();
        targetPosition.z = reader.Read<int16_t>();
        break;
        
    case CommandType::ToggleFence:
        buildingId = reader.Read<int32_t>();
        break;
        
    case CommandType::RunProgram:
        programId = reader.Read<uint8_t>();
        break;
        
    case CommandType::TargetProgram:
        programId = reader.Read<uint8_t>();
        targetPosition.x = reader.Read<int16_t>();
        targetPosition.y = reader.Read<int16_t>();
        targetPosition.z = reader.Read<int16_t>();
        break;
        
    default:
        break;
    }
}

//=============================================================================
// EntitySpawnPacket
//=============================================================================

EntitySpawnPacket::EntitySpawnPacket()
    : entityId(INVALID_NET_ENTITY_ID)
    , entityType(0)
    , teamId(255)
    , position()
    , initialHealth(0)
{}

void EntitySpawnPacket::Write(DataWriter& writer) const
{
    writer.WriteUInt16(entityId);
    writer.WriteByte(entityType);
    writer.WriteByte(teamId);
    writer.WriteInt16(position.x);
    writer.WriteInt16(position.y);
    writer.WriteInt16(position.z);
    writer.WriteUInt16(initialHealth);
}

void EntitySpawnPacket::Read(DataReader& reader)
{
    entityId = reader.Read<uint16_t>();
    entityType = reader.Read<uint8_t>();
    teamId = reader.Read<uint8_t>();
    position.x = reader.Read<int16_t>();
    position.y = reader.Read<int16_t>();
    position.z = reader.Read<int16_t>();
    initialHealth = reader.Read<uint16_t>();
}

//=============================================================================
// EntityDestroyPacket
//=============================================================================

EntityDestroyPacket::EntityDestroyPacket()
    : entityId(INVALID_NET_ENTITY_ID)
    , reason(0)
{}

void EntityDestroyPacket::Write(DataWriter& writer) const
{
    writer.WriteUInt16(entityId);
    writer.WriteByte(reason);
}

void EntityDestroyPacket::Read(DataReader& reader)
{
    entityId = reader.Read<uint16_t>();
    reason = reader.Read<uint8_t>();
}
