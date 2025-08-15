// Negotiation engine with multiple personas.
const $ = (sel)=>document.querySelector(sel);
const transcriptEl = $("#transcript");
const statusEl = $("#status");

function say(who, text) {
  const li = document.createElement("li");
  li.className = "bubble";
  li.innerHTML = `<div class="who">${who}</div><div class="msg">${text}</div>`;
  transcriptEl.appendChild(li);
  transcriptEl.scrollTop = transcriptEl.scrollHeight;
}

function currency(v){ return `₹${v.toFixed(0)}`; }

function clamp(v, min, max){ return Math.min(Math.max(v, min), max); }

// Personas affect concession patterns and language
const Personas = {
  diplomat: {
    name: "Smooth Diplomat",
    buyerStep: 0.25, sellerStep: 0.25,
    styleBuyer: (p)=>`I value quality and fairness. Could we do ${currency(p)} per kg?`,
    styleSeller: (p)=>`Happy to find common ground. How about ${currency(p)} per kg?`,
    accept: ()=>`Deal! delighted to shake on it.`,
    reject: ()=>`Perhaps next time—we couldn't quite align today.`
  },
  aggressive: {
    name: "Aggressive Trader",
    buyerStep: 0.15, sellerStep: 0.35,
    styleBuyer: (p)=>`Best I can do right now: ${currency(p)} per kg. Take it or leave it.`,
    styleSeller: (p)=>`Listen, it's moving fast at ${currency(p)} per kg. This is firm.`,
    accept: ()=>`Done. Cash out, no fuss.`,
    reject: ()=>`No deal—numbers don't work.`
  },
  analyst: {
    name: "Data-Driven Analyst",
    buyerStep: 0.35, sellerStep: 0.35,
    styleBuyer: (p, ctx)=>`Based on last week's market near ${currency(ctx.market)}, ${currency(p)} per kg is rational.`,
    styleSeller: (p, ctx)=>`Costs and margin require around ${currency(p)} per kg vs market ${currency(ctx.market)}.`,
    accept: ()=>`Agreement reached based on data-backed midpoint.`,
    reject: ()=>`Rejecting due to variance exceeding tolerance.`
  },
  wildcard: {
    name: "Creative Wildcard",
    buyerStep: 0.40, sellerStep: 0.30,
    styleBuyer: (p, ctx)=>`Let's bundle! ${ctx.qty}kg at ${currency(p)} per kg and toss in a sample?`,
    styleSeller: (p)=>`Spice it up: ${currency(p)} per kg and a recipe card—fun and fair?`,
    accept: ()=>`Yay! Let's make mangonadas.`,
    reject: ()=>`Universe says not today—no deal.`
  }
};

function rotatePersona(round) {
  const keys = ["diplomat","aggressive","analyst","wildcard"];
  return Personas[keys[round % keys.length]];
}

// Core engine
function negotiate(ctx) {
  const log = [];
  const rounds = ctx.rounds;
  let buyerOffer = ctx.buyerPrice;
  let sellerAsk = ctx.sellerPrice;
  const buyerMin = Math.min(ctx.buyerPrice, ctx.market * 0.98); // buyer won't go above target; will inch toward market
  const sellerMin = Math.min(ctx.sellerPrice, Math.max(ctx.market * 0.96, ctx.sellerPrice * 0.8));

  say("System", `Persona: ${ctx.personaName}. Product: ${ctx.product}, Qty: ${ctx.qty}kg. Market reference: ${currency(ctx.market)} / kg.`);

  for (let r=0; r<rounds; r++) {
    const persona = ctx.persona === "combo" ? rotatePersona(r) : Personas[ctx.persona];
    const buyerStep = persona.buyerStep;
    const sellerStep = persona.sellerStep;

    // Buyer speaks
    const proposeBuyer = clamp(buyerOffer, 1, 999999);
    say("Buyer", persona.styleBuyer(proposeBuyer, ctx));
    log.push({who:"buyer", price: proposeBuyer});

    // Seller reacts
    const targetSeller = Math.max(ctx.market, sellerAsk);
    const concedeSeller = (targetSeller - ctx.market) * sellerStep;
    const sellerCounter = Math.max(ctx.market, targetSeller - concedeSeller);
    say("Seller", persona.styleSeller(sellerCounter, ctx));
    log.push({who:"seller", price: sellerCounter});

    // Check agreement window (midpoint negotiation)
    const mid = (proposeBuyer + sellerCounter) / 2;
    const spread = Math.abs(sellerCounter - proposeBuyer);
    const ok = (spread <= Math.max(2, ctx.market * 0.03)); // within tolerance

    if (ok) {
      const dealPrice = Math.round(mid);
      say("System", `Accepted at midpoint ${currency(dealPrice)} per kg.`);
      return { ok:true, price: dealPrice, log };
    }

    // Update next round offers toward each other bounded by market influence
    buyerOffer = Math.min(proposeBuyer + Math.max(1, (ctx.market - proposeBuyer) * buyerStep), ctx.market);
    sellerAsk = Math.max(sellerCounter - Math.max(1, (sellerCounter - ctx.market) * sellerStep), ctx.market);
  }

  // No deal after rounds; decide final outcome based on closest distance to market
  const lastBuyer = log.filter(x=>x.who==="buyer").slice(-1)[0]?.price ?? ctx.buyerPrice;
  const lastSeller = log.filter(x=>x.who==="seller").slice(-1)[0]?.price ?? ctx.sellerPrice;
  const buyerDist = Math.abs(ctx.market - lastBuyer);
  const sellerDist = Math.abs(lastSeller - ctx.market);

  if (buyerDist < sellerDist) {
    // Seller accepts reluctantly at market
    say("Seller", `Alright, final call at market ${currency(ctx.market)} per kg.`);
    say("Buyer", `Agreed.`);
    return { ok: true, price: Math.round(ctx.market), log };
  }

  say("System", `No agreement within ${rounds} rounds.`);
  return { ok:false, price: null, log };
}

function run() {
  transcriptEl.innerHTML = "";
  statusEl.innerHTML = "";

  const ctx = {
    product: $("#product").value.trim() || "product",
    qty: parseFloat($("#qty").value),
    buyerPrice: parseFloat($("#buyerPrice").value),
    sellerPrice: parseFloat($("#sellerPrice").value),
    market: parseFloat($("#marketPrice").value),
    rounds: parseInt($("#rounds").value, 10),
    persona: $("#persona").value
  };
  ctx.personaName = ctx.persona==="combo" ? "Persona Mix" : Personas[ctx.persona].name;

  const result = negotiate(ctx);

  if (result.ok) {
    const total = Math.round(result.price * ctx.qty);
    statusEl.innerHTML = `<span class="ok">Outcome: Buyer buys the ${ctx.product}.</span> Final price: <span class="code">${currency(result.price)} / kg</span> • Qty: ${ctx.qty}kg • Total: <span class="code">${currency(total)}</span>`;
  } else {
    statusEl.innerHTML = `<span class="no">Outcome: No deal.</span> Try adjusting the persona or tolerance.`;
  }
}

$("#run").addEventListener("click", run);
$("#reset").addEventListener("click", ()=>{ transcriptEl.innerHTML = ""; statusEl.innerHTML=""; });

// Auto-run the example in the prompt: 2kg mango, buyer wants 250 total (₹125/kg), seller asks ₹270 total (₹135/kg), market differs (₹130/kg).
window.addEventListener("load", run);
