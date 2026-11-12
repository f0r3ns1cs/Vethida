import {
  PermissionFlagsBits,
  OverwriteType,
  GuildMember,
  PermissionsBitField,
} from "discord.js";
import type { OverwriteResolvable, APIInteractionGuildMember } from "discord.js";

const P = PermissionFlagsBits;

export function denyView(roleId: string): OverwriteResolvable {
  return { id: roleId, type: OverwriteType.Role, deny: [P.ViewChannel] };
}

export function viewOnly(roleId: string): OverwriteResolvable {
  return {
    id: roleId,
    type: OverwriteType.Role,
    allow: [P.ViewChannel, P.ReadMessageHistory],
    deny: [P.SendMessages, P.CreatePublicThreads, P.CreatePrivateThreads],
  };
}

export function viewAndSend(roleId: string): OverwriteResolvable {
  return {
    id: roleId,
    type: OverwriteType.Role,
    allow: [
      P.ViewChannel,
      P.SendMessages,
      P.ReadMessageHistory,
      P.AddReactions,
      P.EmbedLinks,
      P.AttachFiles,
      P.CreatePublicThreads,
    ],
  };
}

export function staffAccess(roleId: string): OverwriteResolvable {
  return {
    id: roleId,
    type: OverwriteType.Role,
    allow: [
      P.ViewChannel,
      P.SendMessages,
      P.ReadMessageHistory,
      P.ManageMessages,
      P.ManageThreads,
      P.EmbedLinks,
      P.AttachFiles,
      P.MentionEveryone,
    ],
  };
}

export function fullStaffAccess(roleId: string): OverwriteResolvable {
  return {
    id: roleId,
    type: OverwriteType.Role,
    allow: [
      P.ViewChannel,
      P.SendMessages,
      P.ReadMessageHistory,
      P.ManageMessages,
      P.ManageThreads,
      P.ManageChannels,
      P.EmbedLinks,
      P.AttachFiles,
      P.MentionEveryone,
      P.KickMembers,
      P.BanMembers,
      P.ModerateMembers,
    ],
  };
}

export function voiceAccess(roleId: string): OverwriteResolvable {
  return {
    id: roleId,
    type: OverwriteType.Role,
    allow: [P.ViewChannel, P.Connect, P.Speak, P.UseVAD, P.Stream],
  };
}

export function denyEveryone(everyoneId: string): OverwriteResolvable {
  return {
    id: everyoneId,
    type: OverwriteType.Role,
    deny: [P.ViewChannel, P.SendMessages, P.CreatePublicThreads, P.CreatePrivateThreads],
  };
}

export function everyoneViewOnly(everyoneId: string): OverwriteResolvable {
  return {
    id: everyoneId,
    type: OverwriteType.Role,
    allow: [P.ViewChannel, P.ReadMessageHistory],
    deny: [
      P.SendMessages,
      P.CreatePublicThreads,
      P.CreatePrivateThreads,
      P.AddReactions,
    ],
  };
}

export function forumAccess(roleId: string): OverwriteResolvable {
  return {
    id: roleId,
    type: OverwriteType.Role,
    allow: [
      P.ViewChannel,
      P.SendMessages,
      P.ReadMessageHistory,
      P.CreatePublicThreads,
      P.SendMessagesInThreads,
      P.AddReactions,
      P.EmbedLinks,
      P.AttachFiles,
    ],
  };
}

type MemberLike = GuildMember | APIInteractionGuildMember | null | undefined;

export function hasPermission(member: MemberLike, perm: bigint): boolean {
  if (!member) {
    return false;
  }
  const perms = member.permissions;
  if (perms instanceof PermissionsBitField) {
    return perms.has(perm);
  }
  if (typeof perms === "string") {
    return new PermissionsBitField(BigInt(perms)).has(perm);
  }
  return false;
}

export const isModerator = (member: MemberLike): boolean =>
  hasPermission(member, P.ModerateMembers) || hasPermission(member, P.Administrator);

export function outranks(actor: GuildMember, target: GuildMember): boolean {
  if (actor.guild.ownerId === actor.id) {
    return true;
  }
  if (actor.guild.ownerId === target.id) {
    return false;
  }
  return actor.roles.highest.comparePositionTo(target.roles.highest) > 0;
}
