/* This file is generated by venus-protocol.  See vn_protocol_driver.h. */

/*
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: MIT
 */

#ifndef VN_PROTOCOL_DRIVER_DESCRIPTOR_UPDATE_TEMPLATE_H
#define VN_PROTOCOL_DRIVER_DESCRIPTOR_UPDATE_TEMPLATE_H

#include "vn_ring.h"
#include "vn_protocol_driver_structs.h"

/* struct VkDescriptorUpdateTemplateEntry */

static inline size_t
vn_sizeof_VkDescriptorUpdateTemplateEntry(const VkDescriptorUpdateTemplateEntry *val)
{
    size_t size = 0;
    size += vn_sizeof_uint32_t(&val->dstBinding);
    size += vn_sizeof_uint32_t(&val->dstArrayElement);
    size += vn_sizeof_uint32_t(&val->descriptorCount);
    size += vn_sizeof_VkDescriptorType(&val->descriptorType);
    size += vn_sizeof_size_t(&val->offset);
    size += vn_sizeof_size_t(&val->stride);
    return size;
}

static inline void
vn_encode_VkDescriptorUpdateTemplateEntry(struct vn_cs_encoder *enc, const VkDescriptorUpdateTemplateEntry *val)
{
    vn_encode_uint32_t(enc, &val->dstBinding);
    vn_encode_uint32_t(enc, &val->dstArrayElement);
    vn_encode_uint32_t(enc, &val->descriptorCount);
    vn_encode_VkDescriptorType(enc, &val->descriptorType);
    vn_encode_size_t(enc, &val->offset);
    vn_encode_size_t(enc, &val->stride);
}

/* struct VkDescriptorUpdateTemplateCreateInfo chain */

static inline size_t
vn_sizeof_VkDescriptorUpdateTemplateCreateInfo_pnext(const void *val)
{
    /* no known/supported struct */
    return vn_sizeof_simple_pointer(NULL);
}

static inline size_t
vn_sizeof_VkDescriptorUpdateTemplateCreateInfo_self(const VkDescriptorUpdateTemplateCreateInfo *val)
{
    size_t size = 0;
    /* skip val->{sType,pNext} */
    size += vn_sizeof_VkFlags(&val->flags);
    size += vn_sizeof_uint32_t(&val->descriptorUpdateEntryCount);
    if (val->pDescriptorUpdateEntries) {
        size += vn_sizeof_array_size(val->descriptorUpdateEntryCount);
        for (uint32_t i = 0; i < val->descriptorUpdateEntryCount; i++)
            size += vn_sizeof_VkDescriptorUpdateTemplateEntry(&val->pDescriptorUpdateEntries[i]);
    } else {
        size += vn_sizeof_array_size(0);
    }
    size += vn_sizeof_VkDescriptorUpdateTemplateType(&val->templateType);
    size += vn_sizeof_VkDescriptorSetLayout(&val->descriptorSetLayout);
    size += vn_sizeof_VkPipelineBindPoint(&val->pipelineBindPoint);
    size += vn_sizeof_VkPipelineLayout(&val->pipelineLayout);
    size += vn_sizeof_uint32_t(&val->set);
    return size;
}

static inline size_t
vn_sizeof_VkDescriptorUpdateTemplateCreateInfo(const VkDescriptorUpdateTemplateCreateInfo *val)
{
    size_t size = 0;

    size += vn_sizeof_VkStructureType(&val->sType);
    size += vn_sizeof_VkDescriptorUpdateTemplateCreateInfo_pnext(val->pNext);
    size += vn_sizeof_VkDescriptorUpdateTemplateCreateInfo_self(val);

    return size;
}

static inline void
vn_encode_VkDescriptorUpdateTemplateCreateInfo_pnext(struct vn_cs_encoder *enc, const void *val)
{
    /* no known/supported struct */
    vn_encode_simple_pointer(enc, NULL);
}

static inline void
vn_encode_VkDescriptorUpdateTemplateCreateInfo_self(struct vn_cs_encoder *enc, const VkDescriptorUpdateTemplateCreateInfo *val)
{
    /* skip val->{sType,pNext} */
    vn_encode_VkFlags(enc, &val->flags);
    vn_encode_uint32_t(enc, &val->descriptorUpdateEntryCount);
    if (val->pDescriptorUpdateEntries) {
        vn_encode_array_size(enc, val->descriptorUpdateEntryCount);
        for (uint32_t i = 0; i < val->descriptorUpdateEntryCount; i++)
            vn_encode_VkDescriptorUpdateTemplateEntry(enc, &val->pDescriptorUpdateEntries[i]);
    } else {
        vn_encode_array_size(enc, 0);
    }
    vn_encode_VkDescriptorUpdateTemplateType(enc, &val->templateType);
    vn_encode_VkDescriptorSetLayout(enc, &val->descriptorSetLayout);
    vn_encode_VkPipelineBindPoint(enc, &val->pipelineBindPoint);
    vn_encode_VkPipelineLayout(enc, &val->pipelineLayout);
    vn_encode_uint32_t(enc, &val->set);
}

static inline void
vn_encode_VkDescriptorUpdateTemplateCreateInfo(struct vn_cs_encoder *enc, const VkDescriptorUpdateTemplateCreateInfo *val)
{
    assert(val->sType == VK_STRUCTURE_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_CREATE_INFO);
    vn_encode_VkStructureType(enc, &(VkStructureType){ VK_STRUCTURE_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_CREATE_INFO });
    vn_encode_VkDescriptorUpdateTemplateCreateInfo_pnext(enc, val->pNext);
    vn_encode_VkDescriptorUpdateTemplateCreateInfo_self(enc, val);
}

static inline size_t vn_sizeof_vkCreateDescriptorUpdateTemplate(VkDevice device, const VkDescriptorUpdateTemplateCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorUpdateTemplate* pDescriptorUpdateTemplate)
{
    const VkCommandTypeEXT cmd_type = VK_COMMAND_TYPE_vkCreateDescriptorUpdateTemplate_EXT;
    const VkFlags cmd_flags = 0;
    size_t cmd_size = vn_sizeof_VkCommandTypeEXT(&cmd_type) + vn_sizeof_VkFlags(&cmd_flags);

    cmd_size += vn_sizeof_VkDevice(&device);
    cmd_size += vn_sizeof_simple_pointer(pCreateInfo);
    if (pCreateInfo)
        cmd_size += vn_sizeof_VkDescriptorUpdateTemplateCreateInfo(pCreateInfo);
    cmd_size += vn_sizeof_simple_pointer(pAllocator);
    if (pAllocator)
        assert(false);
    cmd_size += vn_sizeof_simple_pointer(pDescriptorUpdateTemplate);
    if (pDescriptorUpdateTemplate)
        cmd_size += vn_sizeof_VkDescriptorUpdateTemplate(pDescriptorUpdateTemplate);

    return cmd_size;
}

static inline void vn_encode_vkCreateDescriptorUpdateTemplate(struct vn_cs_encoder *enc, VkCommandFlagsEXT cmd_flags, VkDevice device, const VkDescriptorUpdateTemplateCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorUpdateTemplate* pDescriptorUpdateTemplate)
{
    const VkCommandTypeEXT cmd_type = VK_COMMAND_TYPE_vkCreateDescriptorUpdateTemplate_EXT;

    vn_encode_VkCommandTypeEXT(enc, &cmd_type);
    vn_encode_VkFlags(enc, &cmd_flags);

    vn_encode_VkDevice(enc, &device);
    if (vn_encode_simple_pointer(enc, pCreateInfo))
        vn_encode_VkDescriptorUpdateTemplateCreateInfo(enc, pCreateInfo);
    if (vn_encode_simple_pointer(enc, pAllocator))
        assert(false);
    if (vn_encode_simple_pointer(enc, pDescriptorUpdateTemplate))
        vn_encode_VkDescriptorUpdateTemplate(enc, pDescriptorUpdateTemplate);
}

static inline size_t vn_sizeof_vkCreateDescriptorUpdateTemplate_reply(VkDevice device, const VkDescriptorUpdateTemplateCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorUpdateTemplate* pDescriptorUpdateTemplate)
{
    const VkCommandTypeEXT cmd_type = VK_COMMAND_TYPE_vkCreateDescriptorUpdateTemplate_EXT;
    size_t cmd_size = vn_sizeof_VkCommandTypeEXT(&cmd_type);

    VkResult ret;
    cmd_size += vn_sizeof_VkResult(&ret);
    /* skip device */
    /* skip pCreateInfo */
    /* skip pAllocator */
    cmd_size += vn_sizeof_simple_pointer(pDescriptorUpdateTemplate);
    if (pDescriptorUpdateTemplate)
        cmd_size += vn_sizeof_VkDescriptorUpdateTemplate(pDescriptorUpdateTemplate);

    return cmd_size;
}

static inline VkResult vn_decode_vkCreateDescriptorUpdateTemplate_reply(struct vn_cs_decoder *dec, VkDevice device, const VkDescriptorUpdateTemplateCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorUpdateTemplate* pDescriptorUpdateTemplate)
{
    VkCommandTypeEXT command_type;
    vn_decode_VkCommandTypeEXT(dec, &command_type);
    assert(command_type == VK_COMMAND_TYPE_vkCreateDescriptorUpdateTemplate_EXT);

    VkResult ret;
    vn_decode_VkResult(dec, &ret);
    /* skip device */
    /* skip pCreateInfo */
    /* skip pAllocator */
    if (vn_decode_simple_pointer(dec)) {
        vn_decode_VkDescriptorUpdateTemplate(dec, pDescriptorUpdateTemplate);
    } else {
        pDescriptorUpdateTemplate = NULL;
    }

    return ret;
}

static inline size_t vn_sizeof_vkDestroyDescriptorUpdateTemplate(VkDevice device, VkDescriptorUpdateTemplate descriptorUpdateTemplate, const VkAllocationCallbacks* pAllocator)
{
    const VkCommandTypeEXT cmd_type = VK_COMMAND_TYPE_vkDestroyDescriptorUpdateTemplate_EXT;
    const VkFlags cmd_flags = 0;
    size_t cmd_size = vn_sizeof_VkCommandTypeEXT(&cmd_type) + vn_sizeof_VkFlags(&cmd_flags);

    cmd_size += vn_sizeof_VkDevice(&device);
    cmd_size += vn_sizeof_VkDescriptorUpdateTemplate(&descriptorUpdateTemplate);
    cmd_size += vn_sizeof_simple_pointer(pAllocator);
    if (pAllocator)
        assert(false);

    return cmd_size;
}

static inline void vn_encode_vkDestroyDescriptorUpdateTemplate(struct vn_cs_encoder *enc, VkCommandFlagsEXT cmd_flags, VkDevice device, VkDescriptorUpdateTemplate descriptorUpdateTemplate, const VkAllocationCallbacks* pAllocator)
{
    const VkCommandTypeEXT cmd_type = VK_COMMAND_TYPE_vkDestroyDescriptorUpdateTemplate_EXT;

    vn_encode_VkCommandTypeEXT(enc, &cmd_type);
    vn_encode_VkFlags(enc, &cmd_flags);

    vn_encode_VkDevice(enc, &device);
    vn_encode_VkDescriptorUpdateTemplate(enc, &descriptorUpdateTemplate);
    if (vn_encode_simple_pointer(enc, pAllocator))
        assert(false);
}

static inline size_t vn_sizeof_vkDestroyDescriptorUpdateTemplate_reply(VkDevice device, VkDescriptorUpdateTemplate descriptorUpdateTemplate, const VkAllocationCallbacks* pAllocator)
{
    const VkCommandTypeEXT cmd_type = VK_COMMAND_TYPE_vkDestroyDescriptorUpdateTemplate_EXT;
    size_t cmd_size = vn_sizeof_VkCommandTypeEXT(&cmd_type);

    /* skip device */
    /* skip descriptorUpdateTemplate */
    /* skip pAllocator */

    return cmd_size;
}

static inline void vn_decode_vkDestroyDescriptorUpdateTemplate_reply(struct vn_cs_decoder *dec, VkDevice device, VkDescriptorUpdateTemplate descriptorUpdateTemplate, const VkAllocationCallbacks* pAllocator)
{
    VkCommandTypeEXT command_type;
    vn_decode_VkCommandTypeEXT(dec, &command_type);
    assert(command_type == VK_COMMAND_TYPE_vkDestroyDescriptorUpdateTemplate_EXT);

    /* skip device */
    /* skip descriptorUpdateTemplate */
    /* skip pAllocator */
}

static inline void vn_submit_vkCreateDescriptorUpdateTemplate(struct vn_ring *vn_ring, VkCommandFlagsEXT cmd_flags, VkDevice device, const VkDescriptorUpdateTemplateCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorUpdateTemplate* pDescriptorUpdateTemplate, struct vn_ring_submit_command *submit)
{
    uint8_t local_cmd_data[VN_SUBMIT_LOCAL_CMD_SIZE];
    void *cmd_data = local_cmd_data;
    size_t cmd_size = vn_sizeof_vkCreateDescriptorUpdateTemplate(device, pCreateInfo, pAllocator, pDescriptorUpdateTemplate);
    if (cmd_size > sizeof(local_cmd_data)) {
        cmd_data = malloc(cmd_size);
        if (!cmd_data)
            cmd_size = 0;
    }
    const size_t reply_size = cmd_flags & VK_COMMAND_GENERATE_REPLY_BIT_EXT ? vn_sizeof_vkCreateDescriptorUpdateTemplate_reply(device, pCreateInfo, pAllocator, pDescriptorUpdateTemplate) : 0;

    struct vn_cs_encoder *enc = vn_ring_submit_command_init(vn_ring, submit, cmd_data, cmd_size, reply_size);
    if (cmd_size) {
        vn_encode_vkCreateDescriptorUpdateTemplate(enc, cmd_flags, device, pCreateInfo, pAllocator, pDescriptorUpdateTemplate);
        vn_ring_submit_command(vn_ring, submit);
        if (cmd_data != local_cmd_data)
            free(cmd_data);
    }
}

static inline void vn_submit_vkDestroyDescriptorUpdateTemplate(struct vn_ring *vn_ring, VkCommandFlagsEXT cmd_flags, VkDevice device, VkDescriptorUpdateTemplate descriptorUpdateTemplate, const VkAllocationCallbacks* pAllocator, struct vn_ring_submit_command *submit)
{
    uint8_t local_cmd_data[VN_SUBMIT_LOCAL_CMD_SIZE];
    void *cmd_data = local_cmd_data;
    size_t cmd_size = vn_sizeof_vkDestroyDescriptorUpdateTemplate(device, descriptorUpdateTemplate, pAllocator);
    if (cmd_size > sizeof(local_cmd_data)) {
        cmd_data = malloc(cmd_size);
        if (!cmd_data)
            cmd_size = 0;
    }
    const size_t reply_size = cmd_flags & VK_COMMAND_GENERATE_REPLY_BIT_EXT ? vn_sizeof_vkDestroyDescriptorUpdateTemplate_reply(device, descriptorUpdateTemplate, pAllocator) : 0;

    struct vn_cs_encoder *enc = vn_ring_submit_command_init(vn_ring, submit, cmd_data, cmd_size, reply_size);
    if (cmd_size) {
        vn_encode_vkDestroyDescriptorUpdateTemplate(enc, cmd_flags, device, descriptorUpdateTemplate, pAllocator);
        vn_ring_submit_command(vn_ring, submit);
        if (cmd_data != local_cmd_data)
            free(cmd_data);
    }
}

static inline VkResult vn_call_vkCreateDescriptorUpdateTemplate(struct vn_ring *vn_ring, VkDevice device, const VkDescriptorUpdateTemplateCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorUpdateTemplate* pDescriptorUpdateTemplate)
{
    VN_TRACE_FUNC();

    struct vn_ring_submit_command submit;
    vn_submit_vkCreateDescriptorUpdateTemplate(vn_ring, VK_COMMAND_GENERATE_REPLY_BIT_EXT, device, pCreateInfo, pAllocator, pDescriptorUpdateTemplate, &submit);
    struct vn_cs_decoder *dec = vn_ring_get_command_reply(vn_ring, &submit);
    if (dec) {
        const VkResult ret = vn_decode_vkCreateDescriptorUpdateTemplate_reply(dec, device, pCreateInfo, pAllocator, pDescriptorUpdateTemplate);
        vn_ring_free_command_reply(vn_ring, &submit);
        return ret;
    } else {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }
}

static inline void vn_async_vkCreateDescriptorUpdateTemplate(struct vn_ring *vn_ring, VkDevice device, const VkDescriptorUpdateTemplateCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorUpdateTemplate* pDescriptorUpdateTemplate)
{
    struct vn_ring_submit_command submit;
    vn_submit_vkCreateDescriptorUpdateTemplate(vn_ring, 0, device, pCreateInfo, pAllocator, pDescriptorUpdateTemplate, &submit);
}

static inline void vn_call_vkDestroyDescriptorUpdateTemplate(struct vn_ring *vn_ring, VkDevice device, VkDescriptorUpdateTemplate descriptorUpdateTemplate, const VkAllocationCallbacks* pAllocator)
{
    VN_TRACE_FUNC();

    struct vn_ring_submit_command submit;
    vn_submit_vkDestroyDescriptorUpdateTemplate(vn_ring, VK_COMMAND_GENERATE_REPLY_BIT_EXT, device, descriptorUpdateTemplate, pAllocator, &submit);
    struct vn_cs_decoder *dec = vn_ring_get_command_reply(vn_ring, &submit);
    if (dec) {
        vn_decode_vkDestroyDescriptorUpdateTemplate_reply(dec, device, descriptorUpdateTemplate, pAllocator);
        vn_ring_free_command_reply(vn_ring, &submit);
    }
}

static inline void vn_async_vkDestroyDescriptorUpdateTemplate(struct vn_ring *vn_ring, VkDevice device, VkDescriptorUpdateTemplate descriptorUpdateTemplate, const VkAllocationCallbacks* pAllocator)
{
    struct vn_ring_submit_command submit;
    vn_submit_vkDestroyDescriptorUpdateTemplate(vn_ring, 0, device, descriptorUpdateTemplate, pAllocator, &submit);
}

#endif /* VN_PROTOCOL_DRIVER_DESCRIPTOR_UPDATE_TEMPLATE_H */
