<script lang="ts">
  import './app.css';
  import { onMount } from 'svelte';
  import { connectWebChannel, type Hub } from './lib/webchannel';
  import { modules, status } from './lib/stores';
  let hub: Hub | null = null; let theme:'dark'|'light'='dark';
  function toggleTheme(){ theme=theme==='dark'?'light':'dark'; document.documentElement.setAttribute('data-theme', theme); }
  onMount(async()=>{
    document.documentElement.setAttribute('data-theme', theme);
    try{
      hub = await connectWebChannel(); status.set('connecté');
      hub.listModules((list)=>{ status.set('modules: '+list.join(', ')); });
      hub.moduleUpdated.connect((name,data)=>{ modules.update(m=>({ ...m, [name]: data })); });
      hub.moduleError.connect((name,err)=>{ modules.update(m=>({ ...m, [name]: { error: String(err) } })); });
    }catch(e){
      status.set('mode dev (pas de WebChannel)');
      modules.set({ uptime:{pretty:'up 12 minutes',loadavg:'0.12 0.16 0.10'}, usb_mics:{sources:'alsa_input.usb-...-mono'} });
    }
  });
</script>
<div class="navbar bg-base-100 shadow">
  <div class="flex-1 px-2 text-lg font-semibold">Hub — Svelte</div>
  <div class="flex-none gap-2 pr-2">
    <span class="opacity-60 mr-2">{$status}</span>
    <button class="btn btn-sm" on:click={toggleTheme}>Theme</button>
  </div>
</div>
<main class="p-4 grid gap-4 md:grid-cols-2 lg:grid-cols-3">
  {#each Object.entries($modules) as [name, data]}
    <section class="card bg-base-100 shadow-xl">
      <div class="card-body">
        <h2 class="card-title">{name}</h2>
        {#if data?.error}<pre class="text-error overflow-auto">{data.error}</pre>
        {:else}<pre class="overflow-auto">{JSON.stringify(data, null, 2)}</pre>{/if}
      </div>
    </section>
  {/each}
</main>
