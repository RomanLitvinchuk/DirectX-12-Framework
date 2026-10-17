#include "DX12App.h"
#include "DirectXColors.h"

void DX12App::DrawPPTonemap(MyTexture* readHDR) {
	commandList->SetPipelineState(renderSystem->ppTonemappingPSO.Get());
	commandList->SetGraphicsRootSignature(renderSystem->ppDefaultRS.Get());

	D3D12_CPU_DESCRIPTOR_HANDLE rtv = renderSystem->postProcess->GetLdrTextureA().rtvHandle;
	commandList->OMSetRenderTargets(1, &rtv, true, nullptr);

	ID3D12DescriptorHeap* descriptorHeaps[] = { renderSystem->postProcess->GetSrvHeap().Get(), renderSystem->samplerHeap.Get()};
	commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
	

	commandList->SetGraphicsRootDescriptorTable(0, readHDR->srvGpuHandle);
	commandList->SetGraphicsRootDescriptorTable(1, renderSystem->samplerHeap->GetGPUDescriptorHandleForHeapStart());

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->DrawInstanced(3, 1, 0, 0);
}

void DX12App::DrawPPVignette(MyTexture* readLDR, MyTexture* writeLDR)
{
	commandList->SetPipelineState(renderSystem->ppVignettePSO.Get());
	commandList->SetGraphicsRootSignature(renderSystem->ppDefaultRS.Get());

	D3D12_CPU_DESCRIPTOR_HANDLE rtv = writeLDR->rtvHandle;
	commandList->OMSetRenderTargets(1, &rtv, true, nullptr);

	ID3D12DescriptorHeap* descriptorHeaps[] = { renderSystem->postProcess->GetSrvHeap().Get(), renderSystem->samplerHeap.Get()};
	commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	commandList->SetGraphicsRootDescriptorTable(0, readLDR->srvGpuHandle);
	commandList->SetGraphicsRootDescriptorTable(1, renderSystem->samplerHeap->GetGPUDescriptorHandleForHeapStart());

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->DrawInstanced(3, 1, 0, 0);
}

void DX12App::DrawPPOutput(MyTexture* readLDR)
{
	commandList->SetPipelineState(renderSystem->ppOutputPSO.Get());
	commandList->SetGraphicsRootSignature(renderSystem->ppDefaultRS.Get());
	commandList->ClearRenderTargetView(GetBackBuffer(), Colors::Black, 0, nullptr);
	D3D12_CPU_DESCRIPTOR_HANDLE bb = GetBackBuffer();
	commandList->OMSetRenderTargets(1, &bb, true, nullptr);
	ID3D12DescriptorHeap* descriptorHeaps[] = { renderSystem->postProcess->GetSrvHeap().Get(), renderSystem->samplerHeap.Get()};
	commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);


	commandList->SetGraphicsRootDescriptorTable(0, readLDR->srvGpuHandle);
	commandList->SetGraphicsRootDescriptorTable(1, renderSystem->samplerHeap->GetGPUDescriptorHandleForHeapStart());

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->DrawInstanced(3, 1, 0, 0);
}
