float4x4 projectMatrix;
float4x4 clipMatrix;
float4 baseColor;
float4 multiplyColor;
float4 screenColor;
float4 channelFlag;
texture mainTexture;
texture maskTexture;

sampler mainSampler = sampler_state {
    texture = <mainTexture>;
};
sampler maskSampler = sampler_state {
    texture = <maskTexture>;
};

struct VS_IN {
    float2 pos : POSITION;
    float2 uv : TEXCOORD0;
};

struct VS_OUT {
    float4 Position : POSITION0;
    float2 uv : TEXCOORD0;
    float4 clipPosition : TEXCOORD1;
};

// Setup mask shader
VS_OUT VertSetupMask(VS_IN In) {
    VS_OUT Out = (VS_OUT)0;
    Out.Position = mul(float4(In.pos, 0.0f, 1.0f), projectMatrix);
    Out.clipPosition = mul(float4(In.pos, 0.0f, 1.0f), projectMatrix);
    Out.uv.x = In.uv.x;
    Out.uv.y = 1.0f - +In.uv.y;
    return Out;
}

float4 PixelSetupMask(VS_OUT In) : COLOR0 {
    float isInside =
    step(baseColor.x, In.clipPosition.x / In.clipPosition.w)
    * step(baseColor.y, In.clipPosition.y / In.clipPosition.w)
    * step(In.clipPosition.x / In.clipPosition.w, baseColor.z)
    * step(In.clipPosition.y / In.clipPosition.w, baseColor.w);
    return channelFlag * tex2D(mainSampler, In.uv).a * isInside;
}

// Vertex Shader
// normal
VS_OUT VertNormal(VS_IN In) {
    VS_OUT Out = (VS_OUT)0;
    Out.Position = mul(float4(In.pos, 0.0f, 1.0f), projectMatrix);
    Out.uv.x = In.uv.x;
    Out.uv.y = 1.0f - +In.uv.y;
    return Out;
}

// masked
VS_OUT VertMasked(VS_IN In) {
    VS_OUT Out = (VS_OUT)0;
    Out.Position = mul(float4(In.pos, 0.0f, 1.0f), projectMatrix);
    Out.clipPosition = mul(float4(In.pos, 0.0f, 1.0f), clipMatrix);
    Out.uv.x = In.uv.x;
    Out.uv.y = 1.0f - In.uv.y;
    return Out;
}

// Pixel Shader
// normal
float4 PixelNormal(VS_OUT In) : COLOR0 {
    float4 texColor = tex2D(mainSampler, In.uv);
    texColor.rgb = texColor.rgb * multiplyColor.rgb;
    texColor.rgb = (texColor.rgb + screenColor.rgb) - (texColor.rgb * screenColor.rgb);
    float4 color = texColor * baseColor;
    color.xyz *= color.w;
    return color;
}

// normal premult alpha
float4 PixelNormalPremult(VS_OUT In) : COLOR0 {
    float4 texColor = tex2D(mainSampler, In.uv);
    texColor.rgb = texColor.rgb * multiplyColor.rgb;
    texColor.rgb = (texColor.rgb + screenColor.rgb * texColor.a) - (texColor.rgb * screenColor.rgb);
    float4 color = texColor * baseColor;
    return color;
}

// masked
float4 PixelMasked(VS_OUT In) : COLOR0{
    float4 texColor = tex2D(mainSampler, In.uv);
    texColor.rgb = texColor.rgb * multiplyColor.rgb;
    texColor.rgb = (texColor.rgb + screenColor.rgb) - (texColor.rgb * screenColor.rgb);
    float4 color = texColor * baseColor;
    color.xyz *= color.w;
    float4 clipMask = (1.0f - tex2D(maskSampler, In.clipPosition.xy / In.clipPosition.w)) * channelFlag;
    float maskVal = clipMask.r + clipMask.g + clipMask.b + clipMask.a;
    color = color * maskVal;
    return color;
}

// masked inverted
float4 PixelMaskedInverted(VS_OUT In) : COLOR0 {
    float4 texColor = tex2D(mainSampler, In.uv);
    texColor.rgb = texColor.rgb * multiplyColor.rgb;
    texColor.rgb = (texColor.rgb + screenColor.rgb) - (texColor.rgb * screenColor.rgb);
    float4 color = texColor * baseColor;
    color.xyz *= color.w;
    float4 clipMask = (1.0f - tex2D(maskSampler, In.clipPosition.xy / In.clipPosition.w)) * channelFlag;
    float maskVal = clipMask.r + clipMask.g + clipMask.b + clipMask.a;
    color = color * (1.0f - maskVal);
    return color;
}

// masked premult alpha
float4 PixelMaskedPremult(VS_OUT In) : COLOR0 {
    float4 texColor = tex2D(mainSampler, In.uv);
    texColor.rgb = texColor.rgb * multiplyColor.rgb;
    texColor.rgb = (texColor.rgb + screenColor.rgb * texColor.a) - (texColor.rgb * screenColor.rgb);
    float4 color = texColor * baseColor;
    float4 clipMask = (1.0f - tex2D(maskSampler, In.clipPosition.xy / In.clipPosition.w)) * channelFlag;
    float maskVal = clipMask.r + clipMask.g + clipMask.b + clipMask.a;
    color = color * maskVal;
    return color;
}

// masked inverted premult alpha
float4 PixelMaskedInvertedPremult(VS_OUT In) : COLOR0 {
    float4 texColor = tex2D(mainSampler, In.uv);
    texColor.rgb = texColor.rgb * multiplyColor.rgb;
    texColor.rgb = (texColor.rgb + screenColor.rgb * texColor.a) - (texColor.rgb * screenColor.rgb);
    float4 color = texColor * baseColor;
    float4 clipMask = (1.0f - tex2D(maskSampler, In.clipPosition.xy / In.clipPosition.w)) * channelFlag;
    float maskVal = clipMask.r + clipMask.g + clipMask.b + clipMask.a;
    color = color * (1.0f - maskVal);
    return color;
}

// Technique
technique ShaderNames_SetupMask {
    pass p0 {
        VertexShader = compile vs_2_0 VertSetupMask();
        PixelShader = compile ps_2_0 PixelSetupMask();
    }
}

technique ShaderNames_Normal {
    pass p0 {
        VertexShader = compile vs_2_0 VertNormal();
        PixelShader = compile ps_2_0 PixelNormal();
    }
}

technique ShaderNames_NormalMasked {
    pass p0 {
        VertexShader = compile vs_2_0 VertMasked();
        PixelShader = compile ps_2_0 PixelMasked();
    }
}

technique ShaderNames_NormalMaskedInverted {
    pass p0 {
        VertexShader = compile vs_2_0 VertMasked();
        PixelShader = compile ps_2_0 PixelMaskedInverted();
    }
}

technique ShaderNames_NormalPremultipliedAlpha {
    pass p0 {
        VertexShader = compile vs_2_0 VertNormal();
        PixelShader = compile ps_2_0 PixelNormalPremult();
    }
}

technique ShaderNames_NormalMaskedPremultipliedAlpha {
    pass p0 {
        VertexShader = compile vs_2_0 VertMasked();
        PixelShader = compile ps_2_0 PixelMaskedPremult();
    }
}

technique ShaderNames_NormalMaskedInvertedPremultipliedAlpha {
    pass p0 {
        VertexShader = compile vs_2_0 VertMasked();
        PixelShader = compile ps_2_0 PixelMaskedInvertedPremult();
    }
}
